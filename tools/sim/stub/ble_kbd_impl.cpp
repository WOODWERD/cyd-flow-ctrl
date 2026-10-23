#include "ble_kbd.h"
bool g_conn=true, g_repair=false;
namespace BleKbd {
 void begin(const char*){} bool connected(){return g_conn;} const char* address(){return "08:3a:f2:69:a1:3e";}
 void forgetPairing(){} bool needsRepair(){return g_repair;} void tick(){}
 void press(uint8_t,uint8_t){} void releaseAll(){} void tap(uint8_t,uint8_t,uint16_t){} void setBattery(uint8_t){} void typeText(const char*,uint16_t){}
}
