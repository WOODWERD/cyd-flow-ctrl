#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <random>
extern uint32_t g_ms;
inline uint32_t millis(){ return g_ms; }
inline void delay(uint32_t){}
#define LOW 0
#define HIGH 1
#define OUTPUT 1
inline void pinMode(int,int){}
inline void digitalWrite(int,int){}
inline void ledcSetup(int,int,int){}
inline void ledcAttachPin(int,int){}
inline void ledcWrite(int,int){}
inline long map(long x,long a,long b,long c,long d){ return (x-a)*(d-c)/(b-a)+c; }
inline uint32_t esp_random(){ static std::mt19937 r(7); return r(); }
struct SerialS { void begin(int){} void println(const char*){} template<class...A> void printf(const char*,A...){} };
static SerialS Serial;
#define VSPI 3
struct SPIClass { SPIClass(int){} void begin(int,int,int,int){} };
#define ADC_0db 0
extern int g_ldr;
inline int analogRead(int){ return g_ldr; }
inline void analogSetPinAttenuation(int,int){}
