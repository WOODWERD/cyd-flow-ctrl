#pragma once
#include <stdint.h>
// In-memory stand-in for the ESP32 NVS Preferences API (the simulator starts from blank NVS).
extern int g_prefRot;
struct Preferences {
  bool begin(const char*, bool){ return true; }
  uint8_t getUChar(const char*, uint8_t def){ return g_prefRot < 0 ? def : (uint8_t)g_prefRot; }
  size_t putUChar(const char*, uint8_t v){ g_prefRot = v; return 1; }
};
