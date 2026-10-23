// Flow Ctrl — a Wispr Flow push-to-talk / hands-free button on the Cheap Yellow Display
// (ESP32-2432S028R). The board pairs with the Mac as a Bluetooth keyboard and sends
// Wispr's documented external-keyboard shortcuts:
//   HOLD TO TALK  →  the push-to-talk chord (default: Ctrl+Opt+T) held while your finger is down
//   HANDS-FREE    →  double-tap of the same chord (Wispr's "double-tap to lock" gesture);
//                    STOP taps the chord once more
//   SEND          →  Return (tap it once the pasted text has landed)
//   DISCARD (trash)  →  Esc during hands-free (cancel), else Cmd+Z / Ctrl+Z (undo the last paste)
// Page 2 "TRANSFORM": Wispr Transforms on the current selection (table below): Prompt Engineer,
//   Drawing note, Client email — plus ⌘A select-all and ⌥O view-diff.
// Match PTT_MODS / PTT_KEY to Wispr Flow → Settings → General → Shortcuts → Push to talk.
// Landscape 320×240. LVGL 9 + TFT_eSPI + XPT2046 + NimBLE.

#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "theme.h"
#include "ble_kbd.h"

LV_IMAGE_DECLARE(ic_dictate);
LV_IMAGE_DECLARE(ic_transform);
LV_IMAGE_DECLARE(ic_handsfree);
LV_IMAGE_DECLARE(logo_woodwerd);   // 240x111 A8 badge, tools/make_logo.py

// ---------------- hardware ----------------
#define SCREEN_W 320
#define SCREEN_H 240
#define XPT_IRQ 36
#define XPT_MOSI 32
#define XPT_MISO 39
#define XPT_CLK 25
#define XPT_CS 33
#define LED_R 4
#define LED_G 16
#define LED_B 17
#define BL_PIN 21
#define LDR_PIN 34           // light sensor (GT36516 LDR) under the lid's light pipe; reads higher when darker
#define BL_CH 0
#ifndef PANEL_ROTATION
#define PANEL_ROTATION 1     // 1 = landscape (USB on the left), 3 = landscape flipped
#endif
#ifndef DIM_AFTER_MS
#define DIM_AFTER_MS 0       // 0 = always full brightness (the CYD is USB-powered, nothing to save).
#endif                       // Set e.g. -DDIM_AFTER_MS=45000 to dim after idle; it never turns off.
#define BL_FULL 255
#define BL_DIM 30
#ifndef BLE_NAME
#define BLE_NAME "Flow Ctrl"
#endif
#include "version.h"

// Host flavour: only key *names* on screen and the Select-all modifier differ. The HID codes the
// board sends are identical; the OS maps Opt→Alt and Cmd→Win.
#ifdef HOST_WIN
#define HOST_NAME   "Windows"
#define OPT_NAME    "Alt"
#define SELALL_MODS MOD_CTRL
#define SELALL_NAME "Ctrl A"
#define UNDO_MODS   MOD_CTRL
#else
#define HOST_NAME   "Mac"
#define OPT_NAME    "Opt"
#define SELALL_MODS MOD_CMD
#define SELALL_NAME "Cmd A"
#define UNDO_MODS   MOD_CMD
#endif

// Chords Wispr Flow listens for (see docs.wisprflow.ai → Supported hotkeys)
#ifndef PTT_MODS
#define PTT_MODS  (MOD_CTRL | MOD_OPT)   // default push-to-talk chord: Ctrl + Opt + T
#endif
#ifndef PTT_KEY
#define PTT_KEY   KEY_T
#endif
#ifndef PTT_LABEL                       // shown on the About page; set it too if you change the chord
#define PTT_LABEL "Ctrl + " OPT_NAME " + T"
#endif
#define DOUBLE_TAP_GAP_MS 120           // gap between the two taps that lock hands-free


// Wispr Transforms (Settings → Transforms). Act on the current selection. Add a row = a new button.
struct Transform { const char* name; const char* hint; uint8_t mods; uint8_t key; };
static const Transform XFORMS[] = {
  { "PROMPT ENGINEER", OPT_NAME " 2  ·  built in",    MOD_OPT, KEY_2 },
  { "DRAWING NOTE",    OPT_NAME " 3  ·  callouts",    MOD_OPT, KEY_3 },
  { "CLIENT EMAIL",    OPT_NAME " 4  ·  client tone", MOD_OPT, KEY_4 },
};
#define N_XFORMS (sizeof(XFORMS) / sizeof(XFORMS[0]))

TFT_eSPI tft = TFT_eSPI(SCREEN_W, SCREEN_H);
SPIClass touchSpi = SPIClass(VSPI);
XPT2046_Touchscreen touch(XPT_CS, XPT_IRQ);
uint16_t tMinX = 300, tMaxX = 3800, tMinY = 300, tMaxY = 3800;

#define DRAW_BUF_SIZE (SCREEN_W * SCREEN_H / 10 * (LV_COLOR_DEPTH / 8))
static uint8_t* draw_buf;
static uint32_t lastTick = 0;

// ---------------- state ----------------
enum Cap { CAP_IDLE, CAP_PTT, CAP_HANDSFREE };
static Cap cap = CAP_IDLE;
static uint32_t capStart = 0;
static bool wasConnected = false;
static int blLevel = BL_FULL;
static int ldrRaw = 0, ldrMin = 4095, ldrMax = 0;   // latest raw ADC reading and the range seen since boot
static float ldrAvg = -1;                          // smoothed (~1 s)
static lv_obj_t* lblLight = nullptr;               // live readout on the About page

// ---------------- widgets ----------------
static lv_obj_t *dot, *lblTitle, *btnDiscard, *lblDiscard;
static lv_obj_t *btnPtt, *lblStatus;          // the black pill IS the talk button
static lv_obj_t *btnHf, *icoHf;
static lv_obj_t *btnSend, *lblSend;
static lv_obj_t *pageDictate, *pageXf, *segKnob, *tabDictate, *tabXf, *icoDictate, *icoXf;
static int knobX[2];
static int page = 0;
static lv_obj_t *toast, *toastTxt;
static uint32_t toastUntil = 0;
#define N_BARS 11
static lv_obj_t* bars[N_BARS];
static int barH[N_BARS];

// ---------------- display / touch glue ----------------
static void flush_cb(lv_display_t* disp, const lv_area_t* a, uint8_t* px) {
  uint32_t w = lv_area_get_width(a), h = lv_area_get_height(a);
  tft.startWrite();
  tft.setAddrWindow(a->x1, a->y1, w, h);
  tft.pushPixels((uint16_t*)px, w * h);
  tft.endWrite();
  lv_display_flush_ready(disp);
}

static void touch_read(lv_indev_t*, lv_indev_data_t* data) {
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    if (p.x < tMinX) tMinX = p.x; if (p.x > tMaxX) tMaxX = p.x;
    if (p.y < tMinY) tMinY = p.y; if (p.y > tMaxY) tMaxY = p.y;
    data->point.x = map(p.x, tMinX, tMaxX, 0, SCREEN_W - 1);
    data->point.y = map(p.y, tMinY, tMaxY, 0, SCREEN_H - 1);
    data->state = LV_INDEV_STATE_PRESSED;
#ifdef TOUCH_DEBUG
    Serial.printf("touch raw %d,%d -> %d,%d\n", p.x, p.y, data->point.x, data->point.y);
#endif
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

static void led(bool r, bool g, bool b) {  // active low
  digitalWrite(LED_R, r ? LOW : HIGH);
  digitalWrite(LED_G, g ? LOW : HIGH);
  digitalWrite(LED_B, b ? LOW : HIGH);
}
static void bl_set(int v) { ledcWrite(BL_CH, v); blLevel = v; }

// ---------------- helpers ----------------
static void show_toast(const char* s, uint32_t ms = 1400) {
  lv_label_set_text(toastTxt, s);
  lv_obj_remove_flag(toast, LV_OBJ_FLAG_HIDDEN);
  toastUntil = millis() + ms;
}

static lv_obj_t* box(lv_obj_t* parent, int x, int y, int w, int h, uint32_t color, int radius);
static lv_obj_t* label(lv_obj_t* parent, const char* s, const lv_font_t* f, uint32_t color);

// ---------------- splash (boot) and About page (long-press the title) ----------------
// Both live on the top layer so they cover whatever page is showing. No swipe, nothing to learn:
// the splash shows the version for two seconds at power-up; the About page is a long press on the
// "Flow Ctrl" title, which is not a button and can't be hit by a heavy finger on HOLD TO TALK.
static lv_obj_t* overlay = nullptr;
#define SPLASH_MS      4000     // how long the boot splash stays before it fades
#define SPLASH_FADE_MS 450
static void overlay_close(lv_event_t*) { if (overlay) { lv_obj_delete(overlay); overlay = nullptr; lblLight = nullptr; } }
static void splash_fade_cb(void*, int32_t v) {          // v: 255 → 0
  if (!overlay) return;
  lv_obj_set_style_bg_opa(overlay, v, 0);
  for (uint32_t i = 0; i < lv_obj_get_child_count(overlay); i++) {
    lv_obj_t* c = lv_obj_get_child(overlay, i);
    lv_obj_set_style_text_opa(c, v, 0);
    lv_obj_set_style_image_opa(c, v, 0);
  }
}
static void splash_fade_done(lv_anim_t*) { overlay_close(nullptr); }
static void splash_timer(lv_timer_t* t) {
  lv_timer_delete(t);
  lv_anim_t a; lv_anim_init(&a);
  lv_anim_set_var(&a, overlay);
  lv_anim_set_values(&a, 255, 0);
  lv_anim_set_duration(&a, SPLASH_FADE_MS);
  lv_anim_set_exec_cb(&a, splash_fade_cb);
  lv_anim_set_completed_cb(&a, splash_fade_done);
  lv_anim_start(&a);
}

static void render_state();
static lv_obj_t* forgetBtn = nullptr; static lv_obj_t* forgetLbl = nullptr; static bool forgetArmed = false;
static void forget_event(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (!forgetArmed) {
    forgetArmed = true;
    lv_label_set_text(forgetLbl, "TAP AGAIN TO FORGET");
    lv_obj_set_style_bg_opa(forgetBtn, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(forgetLbl, C(C_ACCENT_TEXT), 0);
    return;
  }
  BleKbd::forgetPairing();
  overlay_close(nullptr);
  show_toast("Pairing forgotten - re-add it on the computer", 3500);
  render_state();
}
// One About row: grey key on the left, value right-aligned, hairline under it.
static void about_row(int y, const char* k, const char* v, bool mono, bool last) {
  lv_obj_t* a = label(overlay, k, F_SMALL, C_TEXT_2);
  lv_obj_set_pos(a, 16, y + 4);
  lv_obj_t* b = label(overlay, v, mono ? F_MONO : F_SMALL, C_TEXT);
  lv_obj_align(b, LV_ALIGN_TOP_RIGHT, -16, y + 4);
  if (!last) box(overlay, 16, y + 25, SCREEN_W - 32, 1, C_BORDER, 0);
}
static void show_about(bool splash) {
  if (overlay) return;
  overlay = box(lv_layer_top(), 0, 0, SCREEN_W, SCREEN_H, C_BG, 0);
  lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
  if (!splash) lv_obj_add_event_cb(overlay, overlay_close, LV_EVENT_CLICKED, nullptr);

  if (splash) {
    // badge logo, then the two things a new owner needs: which firmware, and which chord to set
    static char ver[40];
    snprintf(ver, sizeof(ver), "Flow Ctrl  ·  v" FW_VERSION "  ·  " HOST_NAME);
    lv_obj_t* logo = lv_image_create(overlay);
    lv_image_set_src(logo, &logo_woodwerd);
    lv_obj_set_style_image_recolor(logo, C(C_TEXT), 0);
    lv_obj_set_style_image_recolor_opa(logo, LV_OPA_COVER, 0);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_t* v = label(overlay, ver, F_SMALL, C_TEXT_2);
    lv_obj_align(v, LV_ALIGN_TOP_MID, 0, 140);
    lv_obj_t* k = label(overlay, "PUSH TO TALK", F_SMALL, C_TEXT_3);
    lv_obj_set_style_text_letter_space(k, 1, 0);
    lv_obj_align(k, LV_ALIGN_TOP_MID, 0, 170);
    lv_obj_t* c = label(overlay, PTT_LABEL, F_TITLE, C_ACCENT_INK);
    lv_obj_align(c, LV_ALIGN_TOP_MID, 0, 190);
    lv_timer_create(splash_timer, SPLASH_MS, nullptr);
    return;
  }

  lv_obj_t* t = label(overlay, "About", F_TITLE, C_TEXT);
  lv_obj_set_pos(t, 16, 14);
  // Forget pairing: red outline, two taps, so a stray finger can't do it.
  forgetArmed = false;
  forgetBtn = lv_button_create(overlay);
  lv_obj_remove_style_all(forgetBtn);
  lv_obj_set_size(forgetBtn, 150, 32);
  lv_obj_align(forgetBtn, LV_ALIGN_TOP_RIGHT, -12, 10);
  lv_obj_set_style_bg_color(forgetBtn, C(C_ACCENT), 0);
  lv_obj_set_style_bg_opa(forgetBtn, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(forgetBtn, 8, 0);
  lv_obj_set_style_border_width(forgetBtn, 1, 0);
  lv_obj_set_style_border_color(forgetBtn, C(C_ACCENT), 0);
  forgetLbl = label(forgetBtn, "FORGET PAIRING", F_SMALL, C_ACCENT_INK);
  lv_obj_center(forgetLbl);
  lv_obj_add_event_cb(forgetBtn, forget_event, LV_EVENT_CLICKED, nullptr);

  static char link[40];
  snprintf(link, sizeof link, BLE_NAME "  ·  %s", BleKbd::connected() ? "connected" : "pairing");
  int y = 50;
  about_row(y, "Version", "v" FW_VERSION "  ·  " HOST_NAME, true, false);  y += 26;
  about_row(y, "Push to talk", PTT_LABEL, true, false);                  y += 26;
  about_row(y, "Hands-free", "tap the hand, tap bar to stop", false, false);   y += 26;
  about_row(y, "Light sensor", "", true, false);
  lblLight = lv_obj_get_child(overlay, -1 - 1);   // the value label about_row just made (the hairline is last)
  y += 26;
  about_row(y, "Bluetooth", link, false, false);                         y += 26;
  about_row(y, "Address", BleKbd::address(), true, true);
  lv_obj_t* footL = label(overlay, "WOODWERD LLC", F_SMALL, C_TEXT_3);
  lv_obj_align(footL, LV_ALIGN_BOTTOM_LEFT, 16, -10);
  lv_obj_t* footR = label(overlay, "tap anywhere to close", F_SMALL, C_TEXT_3);
  lv_obj_align(footR, LV_ALIGN_BOTTOM_RIGHT, -16, -10);
}
static void title_event(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) show_about(false);
}

static lv_obj_t* box(lv_obj_t* parent, int x, int y, int w, int h, uint32_t color, int radius) {
  lv_obj_t* o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_pos(o, x, y); lv_obj_set_size(o, w, h);
  lv_obj_set_style_bg_color(o, C(color), 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(o, radius, 0);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);   // plain shapes; buttons are lv_button
  return o;
}
static lv_obj_t* label(lv_obj_t* parent, const char* s, const lv_font_t* f, uint32_t color) {
  lv_obj_t* l = lv_label_create(parent);
  lv_label_set_text(l, s);
  lv_obj_set_style_text_font(l, f, 0);
  lv_obj_set_style_text_color(l, C(color), 0);
  return l;
}

// ---------------- state → screen ----------------
static void dim(lv_obj_t* o, bool d) { lv_obj_set_style_opa(o, d ? LV_OPA_40 : LV_OPA_COVER, 0); }

static void render_state() {
  const bool on = BleKbd::connected();
  const bool ptt = cap == CAP_PTT, hf = cap == CAP_HANDSFREE;

  // top bar: dot + title
  lv_obj_set_style_bg_color(dot, C(!on ? C_WARN : hf ? C_ACCENT : C_OK), 0);
  lv_label_set_text(lblTitle, !on ? "Pairing…" : "Flow Ctrl");   // the red dot + red chip say hands-free

  // the line under the pill
  static char buf[40];
  lv_obj_set_style_text_font(lblStatus, F_TINY, 0);
  lv_obj_set_style_text_color(lblStatus, C(on ? C_TEXT : C_TEXT_2), 0);
  if (!on) {
    lv_label_set_text(lblStatus, BleKbd::needsRepair() ? "Remove Flow Ctrl on the computer, then add it again"
                                                       : "Add Flow Ctrl in Bluetooth settings");
  } else if (ptt) {
    lv_label_set_text(lblStatus, "LISTENING  ·  RELEASE TO PASTE");
  } else if (hf) {
    uint32_t sec = (millis() - capStart) / 1000;
    snprintf(buf, sizeof buf, "%lu:%02lu  ·  TAP TO STOP", (unsigned long)(sec / 60), (unsigned long)(sec % 60));
    lv_label_set_text(lblStatus, buf);
  } else {
    lv_label_set_text(lblStatus, "HOLD TO TALK");
  }

  // talk pill: a ring around it while held
  lv_obj_set_style_outline_width(btnPtt, ptt ? 3 : 0, 0);

  // SEND steps back while holding; the hands-free chip goes solid red while a session runs
  dim(btnSend, ptt); dim(btnHf, ptt);
  lv_obj_set_style_bg_opa(btnHf, hf ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
  lv_obj_set_style_image_recolor(icoHf, C(hf ? C_ACCENT_TEXT : C_ACCENT_INK), 0);
  lv_obj_set_style_image_opa(icoHf, on ? LV_OPA_COVER : LV_OPA_40, 0);

  // trash chip: becomes a red ✕ (cancel) during hands-free
  lv_label_set_text(lblDiscard, hf ? LV_SYMBOL_CLOSE : LV_SYMBOL_TRASH);
  lv_obj_set_style_bg_color(btnDiscard, C(hf ? C_ACCENT_TINT : C_SURFACE), 0);
  lv_obj_set_style_text_color(lblDiscard, C(hf ? C_ACCENT_INK : C_TEXT_2), 0);

  // disable buttons until paired
  if (on) { lv_obj_remove_state(btnPtt, LV_STATE_DISABLED); lv_obj_remove_state(btnHf, LV_STATE_DISABLED); lv_obj_remove_state(btnSend, LV_STATE_DISABLED); lv_obj_remove_state(btnDiscard, LV_STATE_DISABLED); }
  else    { lv_obj_add_state(btnPtt, LV_STATE_DISABLED);    lv_obj_add_state(btnHf, LV_STATE_DISABLED);    lv_obj_add_state(btnSend, LV_STATE_DISABLED);    lv_obj_add_state(btnDiscard, LV_STATE_DISABLED); }

#ifndef LED_QUIET
  if (!on) led(false, false, ((millis() / 500) % 4) == 0);   // slow blue blink while pairing
  else led(cap != CAP_IDLE, false, false);                   // red while capturing
#endif
}

// ---------------- waveform ----------------
// Idle: a row of dots, like Wispr's bar at rest. Live: centre-weighted bars, white while held,
// red while hands-free.
static void wave_tick(lv_timer_t*) {
  const bool live = cap != CAP_IDLE;
  const int maxH = 54;
  for (int i = 0; i < N_BARS; i++) {
    int target;
    if (live) {
      int envelope = 14 + (maxH - 14) * (N_BARS / 2 - abs(i - N_BARS / 2)) / (N_BARS / 2);
      target = 8 + (int)(esp_random() % (uint32_t)envelope);
    } else {
      target = 6;
    }
    barH[i] += (target - barH[i]) / 2;
    lv_obj_set_height(bars[i], barH[i]);
    lv_obj_set_style_bg_color(bars[i], C(live ? (cap == CAP_HANDSFREE ? C_ACCENT : C_WAVE_LIVE) : C_WAVE_IDLE), 0);
  }
}

// ---------------- key actions ----------------
static void ptt_start() {
  if (cap == CAP_HANDSFREE) return;            // a tap on the pill stops hands-free (ptt_event)
  cap = CAP_PTT; capStart = millis();
  BleKbd::press(PTT_MODS);            // modifiers first, like a real keyboard
  delay(12);
  BleKbd::press(PTT_MODS, PTT_KEY);
  render_state();
}
static void ptt_end() {
  if (cap != CAP_PTT) return;
  BleKbd::releaseAll();
  cap = CAP_IDLE;
  render_state();
}
static void chord_tap() {
  // Real-keyboard order: modifiers first, then the key, then release.
  BleKbd::press(PTT_MODS);
  delay(12);
  BleKbd::press(PTT_MODS, PTT_KEY);
  delay(45);
  BleKbd::releaseAll();
}
static void hf_toggle() {
  if (cap == CAP_PTT) return;
  if (cap == CAP_HANDSFREE) {
    chord_tap();                         // one tap of the PTT chord ends the hands-free session
    cap = CAP_IDLE; show_toast("Pasted");
  } else {
    chord_tap(); delay(DOUBLE_TAP_GAP_MS); chord_tap();   // double-tap = Wispr's lock gesture
    cap = CAP_HANDSFREE; capStart = millis();
  }
  render_state();
}
static void send_return() {
  BleKbd::tap(0, KEY_ENTER);
  show_toast("Sent");
}
// The trash means two things, matching what you'd reach for:
//  - hands-free running: Esc cancels the open Wispr session, nothing pastes
//  - otherwise: Undo (Cmd+Z / Ctrl+Z) removes the text the last hold just pasted
static void discard() {
  if (cap == CAP_PTT) return;                  // never fire under a held chord
  if (cap == CAP_HANDSFREE) {
    BleKbd::tap(0, KEY_ESC);
    cap = CAP_IDLE;
    show_toast("Cancelled — nothing sent");
  } else {
    BleKbd::press(UNDO_MODS); delay(12); BleKbd::press(UNDO_MODS, KEY_Z); delay(45); BleKbd::releaseAll();
    show_toast("Undo");
  }
  render_state();
}

static void ptt_event(lv_event_t* e) {
  switch (lv_event_get_code(e)) {
    case LV_EVENT_PRESSED:     ptt_start(); break;
    case LV_EVENT_SHORT_CLICKED: if (cap == CAP_HANDSFREE) hf_toggle(); break;
    case LV_EVENT_RELEASED:
    case LV_EVENT_PRESS_LOST:  ptt_end(); break;
    default: break;
  }
}
static void hf_event(lv_event_t* e) {
  const lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_SHORT_CLICKED) hf_toggle();
  else if (c == LV_EVENT_LONG_PRESSED) {   // resync without sending anything (e.g. you stopped it from the Mac)
    if (cap == CAP_HANDSFREE) { cap = CAP_IDLE; show_toast("Reset — nothing sent"); render_state(); }
  }
}
static void send_event(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) send_return();
}
static void discard_event(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) discard();
}

// ---------------- build ----------------
// ---------------- pages ----------------
static void set_page(int p) {
  if (cap == CAP_PTT) return;                      // never switch out from under a held chord
  page = p;
  if (p == 0) { lv_obj_remove_flag(pageDictate, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(pageXf, LV_OBJ_FLAG_HIDDEN); }
  else        { lv_obj_add_flag(pageDictate, LV_OBJ_FLAG_HIDDEN);    lv_obj_remove_flag(pageXf, LV_OBJ_FLAG_HIDDEN); }
  lv_anim_t a; lv_anim_init(&a);
  lv_anim_set_var(&a, segKnob);
  lv_anim_set_values(&a, lv_obj_get_x(segKnob), knobX[p]);
  lv_anim_set_duration(&a, 180);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_set_exec_cb(&a, [](void* o, int32_t v) { lv_obj_set_x((lv_obj_t*)o, v); });
  lv_anim_start(&a);
  lv_obj_set_style_image_recolor(icoDictate, C(p == 0 ? C_BG : C_TEXT_2), 0);
  lv_obj_set_style_image_recolor(icoXf,      C(p == 1 ? C_BG : C_TEXT_2), 0);
}
static void tab_event(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) set_page((int)(intptr_t)lv_event_get_user_data(e));
}
static void xf_event(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  const Transform* t = (const Transform*)lv_event_get_user_data(e);
  BleKbd::press(t->mods); delay(12); BleKbd::press(t->mods, t->key); delay(45); BleKbd::releaseAll();
  show_toast(t->name);
}
static void select_all_event(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  BleKbd::press(SELALL_MODS); delay(12); BleKbd::press(SELALL_MODS, KEY_A); delay(45); BleKbd::releaseAll();
  show_toast("Selected all");
}
static void diff_event(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  BleKbd::press(MOD_OPT); delay(12); BleKbd::press(MOD_OPT, KEY_O); delay(45); BleKbd::releaseAll();
  show_toast("View diff");
}

static lv_obj_t* make_button(lv_obj_t* parent, int x, int y, int w, int h) {
  lv_obj_t* b = lv_button_create(parent);
  lv_obj_remove_style_all(b);
  lv_obj_set_pos(b, x, y); lv_obj_set_size(b, w, h);
  lv_obj_set_style_bg_color(b, C(C_SURFACE), 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(b, C(C_SURFACE_HI), LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(b, LV_OPA_40, LV_STATE_DISABLED);
  lv_obj_set_style_radius(b, 10, 0);
  lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  return b;
}

// Layout, landscape 320 x 240:
//   top bar    y 6–46   dot + title (long-press → About) · hands-free chip · trash chip · [dictate | transform] switch
//   Dictate    y 54–132 the black pill = HOLD TO TALK, waveform inside
//              y 142    status line, 12 px (HOLD TO TALK / LISTENING / m:ss / pairing help)
//              y 166–232 SEND, full width. During hands-free a tap on the pill stops it.
//   Transform  y 54–174 2 x 2 transforms (first one black) · y 182–222 SELECT ALL · VIEW DIFF
static void ui_build() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, C(C_BG), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  // --- pages (full-screen transparent containers; the top bar and toast live on scr) ---
  pageDictate = box(scr, 0, 0, SCREEN_W, SCREEN_H, C_BG, 0); lv_obj_set_style_bg_opa(pageDictate, LV_OPA_TRANSP, 0);
  pageXf      = box(scr, 0, 0, SCREEN_W, SCREEN_H, C_BG, 0); lv_obj_set_style_bg_opa(pageXf, LV_OPA_TRANSP, 0);

  // --- top bar ---
  dot = box(scr, 14, 22, 8, 8, C_WARN, 4);
  lblTitle = label(scr, "Flow Ctrl", F_SMALL, C_TEXT_2);
  lv_obj_set_pos(lblTitle, 28, 17);
  lv_obj_t* titleHit = box(scr, 0, 0, 140, 50, C_BG, 0);      // long-press → About page
  lv_obj_set_style_bg_opa(titleHit, LV_OPA_TRANSP, 0);
  lv_obj_add_flag(titleHit, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(titleHit, title_event, LV_EVENT_LONG_PRESSED, nullptr);

  const int barY = 6, barHt = 40, segW = 92, xSeg = SCREEN_W - 12 - segW, wTrash = 48, xTrash = xSeg - 6 - wTrash,
            wHfc = 48, xHf = xTrash - 6 - wHfc;
  auto icon = [&](lv_obj_t* parent, const lv_image_dsc_t* src, uint32_t color) {
    lv_obj_t* im = lv_image_create(parent);
    lv_image_set_src(im, src);
    lv_obj_set_style_image_recolor(im, C(color), 0);
    lv_obj_set_style_image_recolor_opa(im, LV_OPA_COVER, 0);
    lv_obj_center(im);
    return im;
  };

  // trash chip: Esc during hands-free, otherwise Undo
  btnDiscard = make_button(scr, xTrash, barY, wTrash, barHt);
  lblDiscard = label(btnDiscard, LV_SYMBOL_TRASH, &lv_font_montserrat_14, C_TEXT_2);
  lv_obj_center(lblDiscard);
  lv_obj_add_event_cb(btnDiscard, discard_event, LV_EVENT_ALL, nullptr);

  // hands-free chip: red outline; solid red while a session runs (tap again, or tap the pill, to stop)
  btnHf = make_button(scr, xHf, barY, wHfc, barHt);
  lv_obj_set_style_bg_color(btnHf, C(C_ACCENT), 0);
  lv_obj_set_style_bg_opa(btnHf, LV_OPA_TRANSP, 0);
  lv_obj_set_style_bg_color(btnHf, C(C_ACCENT_TINT), LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(btnHf, LV_OPA_COVER, LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(btnHf, LV_OPA_TRANSP, LV_STATE_DISABLED);
  lv_obj_set_style_border_opa(btnHf, LV_OPA_40, LV_STATE_DISABLED);
  lv_obj_set_style_image_opa(btnHf, LV_OPA_40, LV_STATE_DISABLED);
  lv_obj_set_style_border_width(btnHf, 2, 0);
  lv_obj_set_style_border_color(btnHf, C(C_ACCENT), 0);
  icoHf = icon(btnHf, &ic_handsfree, C_ACCENT_INK);
  lv_obj_add_event_cb(btnHf, hf_event, LV_EVENT_ALL, nullptr);

  // page switch: a lozenge, two icons, a black knob under the active one
  const int inset = 4, halfW = (segW - 2 * inset) / 2, knobH = barHt - 2 * inset;
  lv_obj_t* seg = box(scr, xSeg, barY, segW, barHt, C_SURFACE, LV_RADIUS_CIRCLE);
  knobX[0] = inset; knobX[1] = inset + halfW;
  segKnob = box(seg, knobX[0], inset, halfW, knobH, C_KNOB, LV_RADIUS_CIRCLE);
  auto half = [&](int x, int idx) {
    lv_obj_t* b = lv_button_create(seg);
    lv_obj_remove_style_all(b);
    lv_obj_set_pos(b, x, 0); lv_obj_set_size(b, halfW, barHt);
    lv_obj_add_event_cb(b, tab_event, LV_EVENT_CLICKED, (void*)(intptr_t)idx);
    return b;
  };
  tabDictate = half(inset, 0);         icoDictate = icon(tabDictate, &ic_dictate, C_BG);
  tabXf      = half(inset + halfW, 1); icoXf      = icon(tabXf, &ic_transform, C_TEXT_2);

  // --- Dictate: the black pill is the talk button, waveform inside ---
  const int px = 12, py = 54, pw = SCREEN_W - 24, ph = 78;
  btnPtt = make_button(pageDictate, px, py, pw, ph);
  lv_obj_set_style_radius(btnPtt, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(btnPtt, C(C_PILL), 0);
  lv_obj_set_style_bg_color(btnPtt, C(C_PILL), LV_STATE_PRESSED);
  lv_obj_set_style_border_width(btnPtt, 1, 0);
  lv_obj_set_style_border_color(btnPtt, C(C_PILL_RIM), 0);
  lv_obj_set_style_outline_color(btnPtt, C(C_TEXT), 0);
  lv_obj_set_style_outline_pad(btnPtt, 3, 0);
  lv_obj_set_style_outline_width(btnPtt, 0, 0);
  const int bw = 6, gap = 7, span = N_BARS * bw + (N_BARS - 1) * gap;
  for (int i = 0; i < N_BARS; i++) {
    barH[i] = 6;
    bars[i] = box(btnPtt, 0, 0, bw, 6, C_WAVE_IDLE, 3);
    lv_obj_align(bars[i], LV_ALIGN_LEFT_MID, (pw - span) / 2 + i * (bw + gap), 0);
  }
  lv_obj_add_event_cb(btnPtt, ptt_event, LV_EVENT_ALL, nullptr);

  lblStatus = label(pageDictate, "HOLD TO TALK", F_TINY, C_TEXT);
  lv_obj_set_width(lblStatus, pw);
  lv_label_set_long_mode(lblStatus, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_style_text_align(lblStatus, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_letter_space(lblStatus, 1, 0);
  lv_obj_set_pos(lblStatus, px, py + ph + 10);

  const int ry = 166, rh = 66;
  btnSend = make_button(pageDictate, px, ry, pw, rh);
  lblSend = label(btnSend, "SEND", F_LABEL, C_TEXT);
  lv_obj_set_style_text_letter_space(lblSend, 1, 0);
  lv_obj_center(lblSend);
  lv_obj_add_event_cb(btnSend, send_event, LV_EVENT_ALL, nullptr);

  // --- Transform: 2 x 2 (table-driven, first one black) + SELECT ALL · VIEW DIFF ---
  {
    const int gx = 12, gy = 54, gw = 146, gh = 58, g = 4;
    for (size_t i = 0; i < N_XFORMS && i < 4; i++) {
      lv_obj_t* b = make_button(pageXf, gx + (i % 2) * (gw + g), gy + (i / 2) * (gh + g), gw, gh);
      const bool lead = i == 0;
      if (lead) {
        lv_obj_set_style_bg_color(b, C(C_PILL), 0);
        lv_obj_set_style_bg_color(b, C(C_TEXT_2), LV_STATE_PRESSED);
      }
      lv_obj_t* n = label(b, XFORMS[i].name, F_SMALL, lead ? C_WAVE_LIVE : C_TEXT);
      lv_obj_align(n, LV_ALIGN_CENTER, 0, -9);
      lv_obj_t* h = label(b, XFORMS[i].hint, F_SMALL, C_TEXT_3);
      lv_obj_align(h, LV_ALIGN_CENTER, 0, 11);
      lv_obj_add_event_cb(b, xf_event, LV_EVENT_CLICKED, (void*)&XFORMS[i]);
    }
    for (size_t i = N_XFORMS; i < 4; i++) {   // empty slot
      lv_obj_t* b = box(pageXf, gx + (i % 2) * (gw + g), gy + (i / 2) * (gh + g), gw, gh, C_BG, 10);
      lv_obj_set_style_bg_opa(b, LV_OPA_TRANSP, 0);
      lv_obj_set_style_border_width(b, 1, 0); lv_obj_set_style_border_color(b, C(C_BORDER), 0);
      lv_obj_t* h = label(b, "EMPTY SLOT", F_SMALL, C_TEXT_3); lv_obj_center(h);
    }
    const int sy = gy + 2 * gh + g + 8, sh = 40;
    auto util = [&](int x, const char* txt, lv_event_cb_t cb) {
      lv_obj_t* b = make_button(pageXf, x, sy, gw, sh);
      lv_obj_set_style_bg_opa(b, LV_OPA_TRANSP, 0);
      lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_STATE_PRESSED);
      lv_obj_set_style_border_width(b, 1, 0); lv_obj_set_style_border_color(b, C(C_BORDER), 0);
      lv_obj_t* l = label(b, txt, F_SMALL, C_TEXT); lv_obj_center(l);
      lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
    };
    util(gx, "SELECT ALL", select_all_event);
    util(gx + gw + g, "VIEW DIFF", diff_event);
  }
  set_page(0);

  // --- toast: a small black pill over the status line ---
  toast = box(scr, 0, 0, SCREEN_W - 24, 30, C_TEXT, 15);
  lv_obj_align(toast, LV_ALIGN_TOP_MID, 0, 134);
  toastTxt = label(toast, "", F_SMALL, C_BG);
  lv_obj_set_width(toastTxt, SCREEN_W - 48);
  lv_label_set_long_mode(toastTxt, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_style_text_align(toastTxt, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(toastTxt);
  lv_obj_add_flag(toast, LV_OBJ_FLAG_HIDDEN);

  render_state();
  show_about(true);   // boot splash
}

// ---------------- housekeeping ----------------
static void ui_tick(lv_timer_t*) {
  // connection changes
  const bool on = BleKbd::connected();
  if (on != wasConnected) {
    wasConnected = on;
    if (!on && cap != CAP_IDLE) cap = CAP_IDLE;   // keys are gone with the link; don't pretend
    show_toast(on ? "Connected to Mac" : "Disconnected", 1800);
  }
  static uint8_t div = 0;
  if (++div % 5 == 0) render_state();             // 250 ms: timer text, dot blink

  if (toastUntil && millis() > toastUntil) { lv_obj_add_flag(toast, LV_OBJ_FLAG_HIDDEN); toastUntil = 0; }

  // light sensor: every 100 ms, smoothed over ~1 s. v0.9.7 only measures and shows it (About page)
  // so the light pipe can be judged; the backlight does not follow it yet.
  if (div % 2 == 0) {
    ldrRaw = analogRead(LDR_PIN);
    ldrAvg = ldrAvg < 0 ? ldrRaw : ldrAvg + (ldrRaw - ldrAvg) * 0.1f;
    if (ldrRaw < ldrMin) ldrMin = ldrRaw;
    if (ldrRaw > ldrMax) ldrMax = ldrRaw;
    if (lblLight) {
      static char lt[40];
      snprintf(lt, sizeof lt, "%4d  (%d–%d)", (int)(ldrAvg + 0.5f), ldrMin, ldrMax);
      lv_label_set_text(lblLight, lt);
    }
#ifdef LDR_DEBUG
    if (div % 20 == 0) Serial.printf("ldr raw %d avg %d min %d max %d\n", ldrRaw, (int)ldrAvg, ldrMin, ldrMax);
#endif
  }

  // backlight: dim when idle, never off; any capture keeps it awake
  if (cap != CAP_IDLE) lv_display_trigger_activity(NULL);
  const int target = (DIM_AFTER_MS > 0 && lv_display_get_inactive_time(NULL) > DIM_AFTER_MS) ? BL_DIM : BL_FULL;
  if (blLevel != target) {
    int step = target > blLevel ? 40 : -5;
    int next = blLevel + step;
    if ((step > 0 && next > target) || (step < 0 && next < target)) next = target;
    bl_set(next);
  }
}

// ---------------- arduino ----------------
void setup() {
  Serial.begin(115200);
  Serial.println("Flow Ctrl v" FW_VERSION " (" HOST_NAME " build)");
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT); led(false, false, false);

  tft.init();
  tft.setRotation(PANEL_ROTATION);
  tft.setSwapBytes(true);
  tft.fillScreen(RGB565(C_BG));
  // Paint the badge straight to the panel before LVGL exists, so the very first thing the glass shows
  // is the splash, not a strip-by-strip reveal of the UI. LVGL redraws the identical pixels moments later.
  {
    const int w = logo_woodwerd.header.w, h = logo_woodwerd.header.h, x0 = (SCREEN_W - w) / 2, y0 = 16;
    static uint16_t line[240];
    const uint8_t* a = (const uint8_t*)logo_woodwerd.data;
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        // blend the badge's alpha from canvas to ink, so the pre-LVGL frame matches the theme
        const uint32_t v = a[y * w + x], bg = C_BG, fg = C_TEXT;
        const uint32_t r = (((bg >> 16) & 255) * (255 - v) + ((fg >> 16) & 255) * v) / 255;
        const uint32_t g = (((bg >> 8) & 255) * (255 - v) + ((fg >> 8) & 255) * v) / 255;
        const uint32_t b = ((bg & 255) * (255 - v) + (fg & 255) * v) / 255;
        line[x] = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
      }
      tft.pushImage(x0, y0 + y, w, 1, line);
    }
  }
  analogSetPinAttenuation(LDR_PIN, ADC_0db);   // 0–~1.1 V: the most sensitive range for this weak divider
  ledcSetup(BL_CH, 5000, 8);
  ledcAttachPin(BL_PIN, BL_CH);
  bl_set(BL_FULL);

  touchSpi.begin(XPT_CLK, XPT_MISO, XPT_MOSI, XPT_CS);
  touch.begin(touchSpi);
  touch.setRotation(PANEL_ROTATION);

  lv_init();
  draw_buf = new uint8_t[DRAW_BUF_SIZE];
  lv_display_t* disp = lv_display_create(SCREEN_W, SCREEN_H);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, flush_cb);
  lv_display_set_buffers(disp, draw_buf, nullptr, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t* indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touch_read);

  ui_build();
  lv_timer_handler();             // draw the full splash (logo + text) before the ~1 s of BLE init
  BleKbd::begin(BLE_NAME);

  lv_timer_create(ui_tick, 50, nullptr);
  lv_timer_create(wave_tick, 70, nullptr);
  lastTick = millis();
}

void loop() {
  lv_tick_inc(millis() - lastTick);
  lastTick = millis();
  lv_timer_handler();
  BleKbd::tick();
  delay(5);
}
