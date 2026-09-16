#include "ble_kbd.h"
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

// Standard boot-compatible keyboard report map, report ID 1:
// [modifiers][reserved][key1..key6]
static const uint8_t REPORT_MAP[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x06,        // Usage (Keyboard)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x05, 0x07,        //   Usage Page (Key Codes)
  0x19, 0xE0,        //   Usage Minimum (224)
  0x29, 0xE7,        //   Usage Maximum (231)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x08,        //   Report Count (8)
  0x81, 0x02,        //   Input (Data, Variable, Absolute) — modifier byte
  0x95, 0x01,        //   Report Count (1)
  0x75, 0x08,        //   Report Size (8)
  0x81, 0x01,        //   Input (Constant) — reserved byte
  0x95, 0x06,        //   Report Count (6)
  0x75, 0x08,        //   Report Size (8)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x65,        //   Logical Maximum (101)
  0x05, 0x07,        //   Usage Page (Key Codes)
  0x19, 0x00,        //   Usage Minimum (0)
  0x29, 0x65,        //   Usage Maximum (101)
  0x81, 0x00,        //   Input (Data, Array) — 6 key codes
  0x05, 0x08,        //   Usage Page (LEDs)
  0x19, 0x01, 0x29, 0x05, 0x95, 0x05, 0x75, 0x01,
  0x91, 0x02,        //   Output (Data, Variable, Absolute) — LED report
  0x95, 0x01, 0x75, 0x03,
  0x91, 0x01,        //   Output (Constant) — padding
  0xC0               // End Collection
};

static NimBLEHIDDevice* hid = nullptr;
static NimBLECharacteristic* input = nullptr;
static volatile bool isConnected = false;

class ServerCB : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*, NimBLEConnInfo& info) override {
    isConnected = true;
    Serial.printf("[ble] connected %s\n", info.getAddress().toString().c_str());
  }
  void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) override {
    isConnected = false;
    Serial.printf("[ble] disconnected (%d) — advertising\n", reason);
    NimBLEDevice::startAdvertising();
  }
  void onAuthenticationComplete(NimBLEConnInfo& info) override {
    Serial.printf("[ble] auth complete, bonded=%d\n", info.isBonded());
  }
};

static void sendReport(uint8_t mods, uint8_t key) {
  if (!input || !isConnected) return;
  uint8_t r[8] = { mods, 0, key, 0, 0, 0, 0, 0 };
  input->setValue(r, sizeof(r));
  input->notify();
}

void BleKbd::begin(const char* deviceName) {
  NimBLEDevice::init(deviceName);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityAuth(true, false, true);   // bonding, no MITM, secure connections
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCB());
  server->advertiseOnDisconnect(true);

  hid = new NimBLEHIDDevice(server);
  hid->setManufacturer("WOODWERD");
  hid->setPnp(0x02, 0xE502, 0xA111, 0x0210);   // vendor source USB, generic ids
  hid->setHidInfo(0x00, 0x01);                  // country 0, flags: remote wake
  hid->setReportMap((uint8_t*)REPORT_MAP, sizeof(REPORT_MAP));
  input = hid->getInputReport(1);
  hid->getOutputReport(1);                      // keyboard LED report (ignored)
  hid->setBatteryLevel(100);
  hid->startServices();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setAppearance(HID_KEYBOARD);
  adv->addServiceUUID(hid->getHidService()->getUUID());
  adv->setName(deviceName);
  adv->enableScanResponse(true);
  adv->start();
  Serial.printf("[ble] advertising as \"%s\"\n", deviceName);
}

bool BleKbd::connected() { return isConnected; }
void BleKbd::press(uint8_t mods, uint8_t key) { sendReport(mods, key); }
void BleKbd::releaseAll() { sendReport(0, 0); }
void BleKbd::tap(uint8_t mods, uint8_t key, uint16_t holdMs) {
  sendReport(mods, key);
  delay(holdMs);
  sendReport(0, 0);
}
void BleKbd::setBattery(uint8_t pct) { if (hid) hid->setBatteryLevel(pct); }

void BleKbd::typeText(const char* text, uint16_t perKeyMs) {
  for (const char* c = text; *c; c++) {
    uint8_t k = 0;
    if (*c >= 'a' && *c <= 'z') k = 0x04 + (*c - 'a');
    else if (*c >= 'A' && *c <= 'Z') k = 0x04 + (*c - 'A');
    else if (*c >= '1' && *c <= '9') k = 0x1E + (*c - '1');
    else if (*c == '0') k = 0x27;
    else if (*c == ' ') k = 0x2C;
    if (!k) continue;
    sendReport(0, k); delay(perKeyMs);
    sendReport(0, 0); delay(perKeyMs);
  }
}
