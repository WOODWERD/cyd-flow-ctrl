// Minimal BLE HID keyboard on NimBLE — just enough to hold modifier chords and tap keys.
#pragma once
#include <Arduino.h>

// HID modifier bits (left-hand versions)
#define MOD_CTRL  0x01
#define MOD_SHIFT 0x02
#define MOD_OPT   0x04   // Alt / Option
#define MOD_CMD   0x08   // GUI / Command
// HID usage codes
#define KEY_SPACE 0x2C
#define KEY_ESC   0x29
#define KEY_T     0x17
#define KEY_ENTER 0x28
#define KEY_A     0x04
#define KEY_O     0x12
#define KEY_2     0x1F
#define KEY_3     0x20
#define KEY_4     0x21
#define KEY_5     0x22

namespace BleKbd {
  void begin(const char* deviceName);
  bool connected();
  // Hold / release a chord. keys = up to 6 usage codes (0 = none).
  void press(uint8_t modifiers, uint8_t key = 0);
  void releaseAll();
  // Press + release in one go (chord tap)
  void tap(uint8_t modifiers, uint8_t key = 0, uint16_t holdMs = 40);
  void setBattery(uint8_t pct);
  // Type plain text (a-z, 0-9, space) one key at a time — used to drive Spotlight
  void typeText(const char* text, uint16_t perKeyMs = 30);
}
