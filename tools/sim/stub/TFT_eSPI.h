#pragma once
#include <Arduino.h>
extern uint16_t g_fb[320*240];
#define TFT_BLACK 0
struct TFT_eSPI { int ax,ay,aw,ah;
  TFT_eSPI(int,int){} void init(){} void setRotation(int){} void setSwapBytes(bool){}
  void fillScreen(uint16_t c){ for(auto&p:g_fb)p=c; }
  void startWrite(){} void endWrite(){}
  void setAddrWindow(int x,int y,int w,int h){ax=x;ay=y;aw=w;ah=h;}
  void pushPixels(uint16_t* px,uint32_t n){ for(uint32_t i=0;i<n;i++){int x=ax+i%aw,y=ay+i/aw; if(x<320&&y<240) g_fb[y*320+x]=px[i];} }
  void pushImage(int x,int y,int w,int h,uint16_t* d){ for(int j=0;j<h;j++)for(int i=0;i<w;i++) g_fb[(y+j)*320+x+i]=d[j*w+i]; }
};
