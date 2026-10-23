#pragma once
#include <Arduino.h>
struct TS_Point{int x,y,z;};
struct XPT2046_Touchscreen{ XPT2046_Touchscreen(int,int){} void begin(SPIClass&){} void setRotation(int){} bool touched(){return false;} TS_Point getPoint(){return {0,0,0};} };
