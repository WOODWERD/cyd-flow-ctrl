#pragma once
#include <Arduino.h>
// Frame buffer big enough for either shape; g_w x g_h is the current logical size (follows setRotation).
extern uint16_t g_fb[320*320];
extern int g_w, g_h;
#define TFT_BLACK 0
struct TFT_eSPI { int ax,ay,aw,ah;
  TFT_eSPI(int,int){} void init(){} void setRotation(int r){ g_w = (r & 1) ? 320 : 240; g_h = (r & 1) ? 240 : 320; } void setSwapBytes(bool){}
  void fillScreen(uint16_t c){ for(int i=0;i<g_w*g_h;i++) g_fb[i]=c; }
  void startWrite(){} void endWrite(){}
  void setAddrWindow(int x,int y,int w,int h){ax=x;ay=y;aw=w;ah=h;}
  void pushPixels(uint16_t* px,uint32_t n){ for(uint32_t i=0;i<n;i++){int x=ax+i%aw,y=ay+i/aw; if(x<g_w&&y<g_h) g_fb[y*g_w+x]=px[i];} }
  void pushImage(int x,int y,int w,int h,uint16_t* d){ for(int j=0;j<h;j++)for(int i=0;i<w;i++) g_fb[(y+j)*g_w+x+i]=d[j*w+i]; }
};
