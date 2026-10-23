uint32_t g_ms = 0;
int g_ldr = 300;
uint16_t g_fb[320*240];
extern bool g_conn, g_repair;
#include "main.cpp"
static void run(int ms){ for(int t=0;t<ms;t+=10){ g_ms+=10; lv_tick_inc(10); lv_timer_handler(); } }
static void dump(const char* name){
  FILE* f=fopen(name,"wb"); fprintf(f,"P6\n320 240\n255\n");
  for(int i=0;i<320*240;i++){ uint16_t c=g_fb[i]; unsigned char rgb[3]={(unsigned char)(((c>>11)&31)*255/31),(unsigned char)(((c>>5)&63)*255/63),(unsigned char)((c&31)*255/31)}; fwrite(rgb,1,3,f);} fclose(f);
}
int main(){
  setup(); run(600); dump("s_splash.ppm");
  run(5000); dump("s_idle.ppm");
  cap=CAP_PTT; capStart=g_ms; render_state(); run(500); dump("s_hold.ppm");
  cap=CAP_IDLE; render_state(); run(300); show_toast("Pasted"); run(100); dump("s_pasted.ppm");
  run(2000); show_toast("Sent"); run(100); dump("s_sent.ppm");
  run(2000); cap=CAP_HANDSFREE; capStart=g_ms-134000; render_state(); run(500); dump("s_hf.ppm");
  cap=CAP_IDLE; render_state(); run(2500); set_page(1); run(500); dump("s_xf.ppm");
  show_toast("PROMPT ENGINEER"); run(100); dump("s_xf_pe.ppm");
  run(2000); set_page(0); run(500); show_about(false); run(300); dump("s_about.ppm");
  overlay_close(nullptr); g_conn=false; render_state(); run(3000); dump("s_pairing.ppm");
  return 0;
}
