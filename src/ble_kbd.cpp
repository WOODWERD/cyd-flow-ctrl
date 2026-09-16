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

static const uint32_t PARAM_REQ_DELAY_MS = 3000;   // settle time after encryption / after a Mac update
static const uint8_t  PARAM_REQ_MAX = 3;           // per connection, so we never ping-pong with the Mac
static NimBLEHIDDevice* hid = nullptr;
static NimBLECharacteristic* input = nullptr;
static volatile bool isConnected = false;

static uint32_t connectedAt = 0;
static uint32_t paramReqAt = 0;        // when to send our conn-param request (0 = nothing pending)
static uint8_t  paramReqCount = 0;     // requests sent on this connection (capped)
static bool     encrypted = false;
static uint16_t connHandle = 0;
static uint16_t curLatency = 0;
static uint16_t curTimeout10ms = 0;    // last reported supervision timeout, 10 ms units
static NimBLEServer* srv = nullptr;
static const char* reasonText(int r) {
  switch (r) {
    case 0x208: return "supervision timeout (link went quiet)";
    case 0x213: return "remote user terminated";
    case 0x216: return "local host terminated";
    case 0x23E: return "connection failed to establish";
    case 0x22E: return "LMP response timeout";
    case 0x228: return "instant passed (missed a channel-map/param update)";
    default: return "";
  }
}

static bool paramNeeded() { return curLatency > 2 || curTimeout10ms < 200; }

class ServerCB : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& info) override {
    isConnected = true;
    connectedAt = millis();
    Serial.printf("[%8lu] [ble] connected %s  interval=%u latency=%u timeout=%ums\n", (unsigned long)millis(),
                  info.getAddress().toString().c_str(), info.getConnInterval(), info.getConnLatency(), info.getConnTimeout() * 10);
    // macOS opens every HID link with a 720 ms supervision timeout and then runs its own
    // parameter update. Asking for ours in the same instant collides with that (0x228 "instant
    // passed" and a drop), so the request is deferred to the main loop - see BleKbd::tick().
    srv = server;
    connHandle = info.getConnHandle();
    curLatency = info.getConnLatency();
    curTimeout10ms = info.getConnTimeout();
    encrypted = false;
    paramReqCount = 0;
    paramReqAt = 0;                                   // armed once the link is encrypted
  }
  void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) override {
    isConnected = false;
    paramReqAt = 0;
    encrypted = false;
    Serial.printf("[%8lu] [ble] disconnected 0x%03X %s after %lus - advertising\n", (unsigned long)millis(), reason,
                  reasonText(reason), (unsigned long)((millis() - connectedAt) / 1000));
    NimBLEDevice::startAdvertising();
  }
  void onAuthenticationComplete(NimBLEConnInfo& info) override {
    Serial.printf("[%8lu] [ble] auth complete, bonded=%d encrypted=%d\n", (unsigned long)millis(), info.isBonded(), info.isEncrypted());
    encrypted = info.isEncrypted();
    if (encrypted && !paramReqAt) paramReqAt = millis() + PARAM_REQ_DELAY_MS;
  }
  void onConnParamsUpdate(NimBLEConnInfo& info) override {
    curLatency = info.getConnLatency();
    curTimeout10ms = info.getConnTimeout();
    // macOS moves a bonded keyboard to latency 22: the board may then sleep through 22 events,
    // miss a channel-map instant and get dropped with 0x228. Ask again for latency 0.
    if (encrypted && !paramReqAt && paramNeeded()) paramReqAt = millis() + PARAM_REQ_DELAY_MS;
    Serial.printf("[%8lu] [ble] conn params now interval=%u latency=%u timeout=%ums\n", (unsigned long)millis(),
                  info.getConnInterval(), info.getConnLatency(), info.getConnTimeout() * 10);
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
  // default TX power: +9 dBm gains nothing at desk range on the CYD PCB antenna
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

void BleKbd::tick() {
  if (!paramReqAt || !isConnected || (int32_t)(millis() - paramReqAt) < 0) return;
  paramReqAt = 0;
  if (!paramNeeded()) {
    Serial.printf("[%8lu] [ble] link params fine (latency=%u timeout=%ums), no request\n", (unsigned long)millis(),
                  curLatency, curTimeout10ms * 10);
    return;
  }
  if (paramReqCount >= PARAM_REQ_MAX) {
    Serial.printf("[%8lu] [ble] Mac keeps its own params; giving up for this connection\n", (unsigned long)millis());
    return;
  }
  paramReqCount++;
  // Apple-compliant HID request, but with NO latency: the board is USB-powered and listening on
  // every 15-30 ms event keeps it from ever missing a channel-map or parameter instant.
  Serial.printf("[%8lu] [ble] request %u/%u: interval 12-24 latency 0 timeout 6000ms\n", (unsigned long)millis(),
                paramReqCount, PARAM_REQ_MAX);
  srv->updateConnParams(connHandle, 12, 24, 0, 600);
}
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
