uint32_t g_ms = 0;
int g_ldr = 300;
uint16_t g_fb[320*320];
int g_w = 320, g_h = 240, g_prefRot = -1;
extern bool g_conn, g_repair;
#include "main.cpp"
static void run(int ms){ for(int t=0;t<ms;t+=10){ g_ms+=10; lv_tick_inc(10); lv_timer_handler(); } }
static void dump(const char* name){
  FILE* f=fopen(name,"wb"); fprintf(f,"P6\n%d %d\n255\n", g_w, g_h);
  for(int i=0;i<g_w*g_h;i++){ uint16_t c=g_fb[i]; unsigned char rgb[3]={(unsigned char)(((c>>11)&31)*255/31),(unsigned char)(((c>>5)&63)*255/63),(unsigned char)((c&31)*255/31)}; fwrite(rgb,1,3,f);} fclose(f);
}
// One pass through every state; prefix "s_" for landscape, "p_" for portrait.
static void tour(const char* pre){
  char n[64];
  #define D(x) (snprintf(n,sizeof n,"%s%s.ppm",pre,x), dump(n))
  run(5000); D("idle");
  cap=CAP_PTT; capStart=g_ms; render_state(); run(500); D("hold");
  cap=CAP_IDLE; render_state(); run(300); show_toast("Pasted"); run(100); D("pasted");
  run(2000); show_toast("Sent"); run(100); D("sent");
  run(2000); cap=CAP_HANDSFREE; capStart=g_ms-134000; render_state(); run(500); D("hf");
  cap=CAP_IDLE; render_state(); run(2500); set_page(1); run(500); D("xf");
  show_toast("PROMPT ENGINEER"); run(100); D("xf_pe");
  run(2000); set_page(0); run(500); show_about(false); run(300); D("about");
  overlay_close(nullptr); g_conn=false; render_state(); run(3000); D("pairing");
  g_conn=true; run(2500);
}
int main(){
  setup(); run(600); dump("s_splash.ppm");
  tour("s_");
  // About › PORTRAIT, exactly as the button does it
  show_about(false); run(100); lv_async_call(rotate_async, (void*)(uintptr_t)2); run(300); dump("p_rotated.ppm");
  tour("p_");
  // About › FLIP in portrait (USB at the top), then a cold boot that comes back in the saved shape
  lv_async_call(rotate_async, (void*)(uintptr_t)0); run(300); dump("p_flipped.ppm");
  lv_async_call(rotate_async, (void*)(uintptr_t)2); run(300);
  lv_obj_clean(lv_layer_top()); overlay=nullptr; lv_obj_clean(lv_screen_active());
  rot = uiPrefs.getUChar("rot", PANEL_ROTATION); set_rotation(rot); ui_build(true); run(600); dump("p_splash.ppm");
  printf("saved rot=%d, screen %dx%d\n", g_prefRot, SCREEN_W, SCREEN_H);
  return 0;
}
