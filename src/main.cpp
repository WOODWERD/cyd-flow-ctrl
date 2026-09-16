// Flow Ctrl — a Wispr Flow push-to-talk / hands-free button on the Cheap Yellow Display
// (ESP32-2432S028R). The board pairs with the Mac as a Bluetooth keyboard and sends
// Wispr's documented external-keyboard shortcuts:
//   HOLD TO TALK  →  the push-to-talk chord (default: Ctrl+Opt+T) held while your finger is down
//   HANDS-FREE    →  double-tap of the same chord (Wispr's "double-tap to lock" gesture);
//                    STOP taps the chord once more
//   SEND          →  Return (tap it once the pasted text has landed)
//   DISCARD (trash)  →  Esc
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

// Chords Wispr Flow listens for (see docs.wisprflow.ai → Supported hotkeys)
#ifndef PTT_MODS
#define PTT_MODS  (MOD_CTRL | MOD_OPT)   // default push-to-talk chord: Ctrl + Opt + T
#endif
#ifndef PTT_KEY
#define PTT_KEY   KEY_T
#endif
#define DOUBLE_TAP_GAP_MS 120           // gap between the two taps that lock hands-free


// Wispr Transforms (Settings → Transforms). Act on the current selection. Add a row = a new button.
struct Transform { const char* name; const char* hint; uint8_t mods; uint8_t key; };
static const Transform XFORMS[] = {
  { "PROMPT ENGINEER", "Opt 2  ·  built in",      MOD_OPT, KEY_2 },
  { "DRAWING NOTE",    "Opt 3  ·  callouts", MOD_OPT, KEY_3 },
  { "CLIENT EMAIL",    "Opt 4  ·  client tone", MOD_OPT, KEY_4 },
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

// ---------------- widgets ----------------
static lv_obj_t *dot, *lblTitle, *btnDiscard;
static lv_obj_t *pill, *lblStatus;
static lv_obj_t *btnPtt, *lblPtt, *micCap, *micCup, *micStem, *micBase;
static lv_obj_t *btnHf, *lblHf, *icoHf;
static lv_obj_t *btnSend, *lblSend, *lblSendSym;
static lv_obj_t *pageDictate, *pageXf, *segKnob, *tabDictate, *tabXf, *icoDictate, *icoXf;
static int knobX[2];
static int page = 0;
static lv_obj_t *toast, *toastTxt;
static uint32_t toastUntil = 0;
#define N_BARS 7
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

static lv_obj_t* box(lv_obj_t* parent, int x, int y, int w, int h, uint32_t color, int radius) {
  lv_obj_t* o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_pos(o, x, y); lv_obj_set_size(o, w, h);
  lv_obj_set_style_bg_color(o, C(color), 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(o, radius, 0);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  return o;
}
static lv_obj_t* label(lv_obj_t* parent, const char* s, const lv_font_t* f, uint32_t color) {
  lv_obj_t* l = lv_label_create(parent);
  lv_label_set_text(l, s);
  lv_obj_set_style_text_font(l, f, 0);
  lv_obj_set_style_text_color(l, C(color), 0);
  return l;
}

// ---------------- button row geometry (Dictate page) ----------------
// idle:       [HOLD TO TALK 112][SEND 112][HF 64]
// hands-free: HOLD slides off left, SEND slides to the left edge, STOP grows to fill the rest
#define ROW_X 12
#define ROW_Y 128
#define ROW_H (SCREEN_H - ROW_Y - 12)
#define W_PTT 112
#define W_SEND 112
#define W_HF 64
#define GAPB 4
#define X_SEND_IDLE (ROW_X + W_PTT + GAPB)
#define X_HF_IDLE (X_SEND_IDLE + W_SEND + GAPB)
#define X_HF_BIG (ROW_X + W_SEND + GAPB)
#define W_HF_BIG (SCREEN_W - ROW_X - X_HF_BIG)
#define ROW_ANIM_MS 220
static bool rowBig = false;

static void anim_x(lv_obj_t* o, int32_t to) {
  lv_anim_t a; lv_anim_init(&a); lv_anim_set_var(&a, o);
  lv_anim_set_values(&a, lv_obj_get_x(o), to); lv_anim_set_duration(&a, ROW_ANIM_MS);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_set_exec_cb(&a, [](void* v, int32_t x) { lv_obj_set_x((lv_obj_t*)v, x); });
  lv_anim_start(&a);
}
static void anim_w(lv_obj_t* o, int32_t to) {
  lv_anim_t a; lv_anim_init(&a); lv_anim_set_var(&a, o);
  lv_anim_set_values(&a, lv_obj_get_width(o), to); lv_anim_set_duration(&a, ROW_ANIM_MS);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_set_exec_cb(&a, [](void* v, int32_t w) { lv_obj_set_width((lv_obj_t*)v, w); });
  lv_anim_start(&a);
}
static void row_layout(bool big) {
  if (big == rowBig) return;
  rowBig = big;
  if (big) {
    anim_x(btnPtt, -W_PTT - 8);
    anim_x(btnSend, ROW_X);
    anim_x(btnHf, X_HF_BIG); anim_w(btnHf, W_HF_BIG);
  } else {
    anim_x(btnPtt, ROW_X);
    anim_x(btnSend, X_SEND_IDLE);
    anim_x(btnHf, X_HF_IDLE); anim_w(btnHf, W_HF);
  }
}

// ---------------- state → screen ----------------
static void render_state() {
  const bool on = BleKbd::connected();

  // connection dot + title
  lv_obj_set_style_bg_color(dot, C(on ? C_OK : C_WARN), 0);
  lv_label_set_text(lblTitle, on ? "Flow Ctrl" : "Pairing…");

  // status line under the pill
  static char buf[48];
  if (!on) {
    lv_label_set_text(lblStatus, "Add \"Flow Ctrl\" in Bluetooth settings");
  } else if (cap == CAP_PTT) {
    lv_label_set_text(lblStatus, "Listening — release to paste");
  } else if (cap == CAP_HANDSFREE) {
    uint32_t s = (millis() - capStart) / 1000;
    snprintf(buf, sizeof buf, "Hands-free  %lu:%02lu", (unsigned long)(s / 60), (unsigned long)(s % 60));
    lv_label_set_text(lblStatus, buf);
  } else {
    lv_label_set_text(lblStatus, "Ready");
  }

  // pill: rim glows while capturing
  lv_obj_set_style_border_color(pill, C(cap == CAP_HANDSFREE ? C_ACCENT : cap == CAP_PTT ? C_TEXT : C_BORDER), 0);

  // PTT button
  const bool pttDown = cap == CAP_PTT;
  lv_obj_set_style_bg_color(btnPtt, C(pttDown ? C_TEXT : C_SURFACE), 0);
  lv_obj_set_style_text_color(lblPtt, C(pttDown ? C_BG : C_TEXT), 0);
  lv_label_set_text(lblPtt, pttDown ? "LISTENING" : "HOLD TO TALK");
  const uint32_t micC = pttDown ? C_BG : C_TEXT;
  lv_obj_set_style_bg_color(micCap, C(micC), 0);
  lv_obj_set_style_border_color(micCup, C(micC), 0);
  lv_obj_set_style_bg_color(micStem, C(micC), 0);
  lv_obj_set_style_bg_color(micBase, C(micC), 0);

  // Hands-free button
  const bool hf = cap == CAP_HANDSFREE;
  lv_obj_set_style_bg_color(btnHf, C(hf ? C_ACCENT : C_SURFACE), 0);
  lv_obj_set_style_text_color(lblHf, C(hf ? C_ACCENT_TEXT : C_TEXT), 0);
  lv_obj_set_style_image_recolor(icoHf, C(hf ? C_ACCENT_TEXT : C_TEXT), 0);
  lv_label_set_text(lblHf, hf ? "STOP" : "");
  lv_obj_set_style_text_font(lblHf, hf ? F_TITLE : F_SMALL, 0);
  lv_obj_align(icoHf, LV_ALIGN_CENTER, 0, hf ? -18 : -10);
  lv_obj_align(lblHf, LV_ALIGN_CENTER, 0, hf ? 18 : 22);
  row_layout(hf);

  // disable buttons until paired
  if (on) { lv_obj_remove_state(btnPtt, LV_STATE_DISABLED); lv_obj_remove_state(btnHf, LV_STATE_DISABLED); lv_obj_remove_state(btnSend, LV_STATE_DISABLED); lv_obj_remove_state(btnDiscard, LV_STATE_DISABLED); }
  else    { lv_obj_add_state(btnPtt, LV_STATE_DISABLED);    lv_obj_add_state(btnHf, LV_STATE_DISABLED);    lv_obj_add_state(btnSend, LV_STATE_DISABLED);    lv_obj_add_state(btnDiscard, LV_STATE_DISABLED); }

#ifndef LED_QUIET
  if (!on) led(false, false, ((millis() / 500) % 4) == 0);   // slow blue blink while pairing
  else led(cap != CAP_IDLE, false, false);                   // red while capturing
#endif
}

// ---------------- waveform ----------------
static void wave_tick(lv_timer_t*) {
  const bool live = cap != CAP_IDLE;
  for (int i = 0; i < N_BARS; i++) {
    int target;
    if (live) {
      // centre-weighted random walk so it reads as speech, not noise
      int envelope = 10 + 18 * (N_BARS / 2 - abs(i - N_BARS / 2)) / (N_BARS / 2);
      target = 6 + (int)(esp_random() % (uint32_t)envelope);
    } else {
      target = 4;
    }
    barH[i] += (target - barH[i]) / 2;
    lv_obj_set_height(bars[i], barH[i]);
    lv_obj_set_style_bg_color(bars[i], C(live ? (cap == CAP_HANDSFREE ? C_ACCENT : C_WAVE_LIVE) : C_WAVE_IDLE), 0);
  }
}

// ---------------- key actions ----------------
static void ptt_start() {
  if (cap == CAP_HANDSFREE) { show_toast("Hands-free is on — tap STOP first"); return; }
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
static void discard() {
  BleKbd::releaseAll();
  BleKbd::tap(0, KEY_ESC);
  cap = CAP_IDLE;
  show_toast("Discarded");
  render_state();
}

static void ptt_event(lv_event_t* e) {
  switch (lv_event_get_code(e)) {
    case LV_EVENT_PRESSED:     ptt_start(); break;
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
  BleKbd::press(MOD_CMD); delay(12); BleKbd::press(MOD_CMD, KEY_A); delay(45); BleKbd::releaseAll();
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
  lv_obj_set_style_radius(b, R_BTN, 0);
  lv_obj_set_style_border_width(b, 1, 0);
  lv_obj_set_style_border_color(b, C(C_BORDER), 0);
  return b;
}

static void ui_build() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, C(C_BG), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  // --- pages (full-screen transparent containers; the top bar and toast live on scr) ---
  pageDictate = box(scr, 0, 0, SCREEN_W, SCREEN_H, C_BG, 0); lv_obj_set_style_bg_opa(pageDictate, LV_OPA_TRANSP, 0);
  pageXf      = box(scr, 0, 0, SCREEN_W, SCREEN_H, C_BG, 0); lv_obj_set_style_bg_opa(pageXf, LV_OPA_TRANSP, 0);

  // --- top bar: status only ---
  dot = box(scr, 16, 14, 8, 8, C_WARN, 4);
  lblTitle = label(scr, "Flow Ctrl", F_SMALL, C_TEXT_2);
  lv_obj_set_pos(lblTitle, 32, 9);

  // --- shared row (both pages): waveform circle · trash · [dictate | transform] lozenge with a sliding knob ---
  const int rowY = 36, D = 56;
  const int xWave = 12, xTrash = xWave + D + 8, xSeg = xTrash + D + 8, segW = SCREEN_W - 12 - xSeg;
  const int pillW = D, pillH = D, pillX = xWave, pillY = rowY;
  pill = box(scr, pillX, pillY, pillW, pillH, C_SURFACE, LV_RADIUS_CIRCLE);
  lv_obj_set_style_border_width(pill, 1, 0);
  lv_obj_set_style_border_color(pill, C(C_BORDER), 0);
  const int bw = 4, gap = 2, span = N_BARS * bw + (N_BARS - 1) * gap;
  for (int i = 0; i < N_BARS; i++) {
    barH[i] = 4;
    bars[i] = box(pill, 0, 0, bw, 4, C_WAVE_IDLE, R_BAR);
    lv_obj_align(bars[i], LV_ALIGN_LEFT_MID, (pillW - span) / 2 + i * (bw + gap), 0);
  }
  auto icon = [&](lv_obj_t* parent, const lv_image_dsc_t* src, uint32_t color) {
    lv_obj_t* im = lv_image_create(parent);
    lv_image_set_src(im, src);
    lv_obj_set_style_image_recolor(im, C(color), 0);
    lv_obj_set_style_image_recolor_opa(im, LV_OPA_COVER, 0);
    lv_obj_center(im);
    return im;
  };

  // Discard (Esc)
  btnDiscard = make_button(scr, xTrash, rowY, D, D);
  lv_obj_set_style_radius(btnDiscard, LV_RADIUS_CIRCLE, 0);
  lv_obj_t* ld = label(btnDiscard, LV_SYMBOL_TRASH, &lv_font_montserrat_14, C_TEXT_2);
  lv_obj_center(ld);
  lv_obj_add_event_cb(btnDiscard, discard_event, LV_EVENT_ALL, nullptr);

  // Page selector: a lozenge, two icons, a green knob that slides under the active one
  const int inset = 4, halfW = (segW - 2 * inset) / 2, knobH = D - 2 * inset;
  lv_obj_t* seg = box(scr, xSeg, rowY, segW, D, C_SURFACE, LV_RADIUS_CIRCLE);
  lv_obj_set_style_border_width(seg, 1, 0); lv_obj_set_style_border_color(seg, C(C_BORDER), 0);
  knobX[0] = inset; knobX[1] = inset + halfW;
  segKnob = box(seg, knobX[0], inset, halfW, knobH, C_OK, LV_RADIUS_CIRCLE);
  auto half = [&](int x, const lv_image_dsc_t* src, int idx) {
    lv_obj_t* b = lv_button_create(seg);
    lv_obj_remove_style_all(b);
    lv_obj_set_pos(b, x, 0); lv_obj_set_size(b, halfW, D);
    lv_obj_add_event_cb(b, tab_event, LV_EVENT_CLICKED, (void*)(intptr_t)idx);
    return b;
  };
  tabDictate = half(inset, &ic_dictate, 0);        icoDictate = icon(tabDictate, &ic_dictate, C_BG);
  tabXf      = half(inset + halfW, &ic_transform, 1); icoXf   = icon(tabXf, &ic_transform, C_TEXT_2);

  lblStatus = label(pageDictate, "Ready", F_SMALL, C_TEXT_2);
  lv_obj_set_width(lblStatus, SCREEN_W - 32);
  lv_obj_set_style_text_align(lblStatus, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(lblStatus, 16, 100);

  // --- buttons ---
  const int by = ROW_Y, bh = ROW_H, bw2 = W_PTT, bwS = W_SEND, bwH = W_HF, bxS = X_SEND_IDLE, bx2 = X_HF_IDLE;  // hold · send equal, hands-free gets the rest
  btnPtt = make_button(pageDictate, 12, by, bw2, bh);
  // mic glyph: capsule + cup + stem + base
  micCap  = box(btnPtt, 0, 0, 12, 22, C_TEXT, 6);   lv_obj_align(micCap,  LV_ALIGN_TOP_MID, 0, 16);
  micCup  = box(btnPtt, 0, 0, 24, 20, C_TEXT, 12);  lv_obj_align(micCup,  LV_ALIGN_TOP_MID, 0, 22);
  lv_obj_set_style_bg_opa(micCup, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(micCup, 2, 0);
  lv_obj_set_style_border_side(micCup, (lv_border_side_t)(LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_BOTTOM), 0);
  lv_obj_set_style_border_color(micCup, C(C_TEXT), 0);
  micStem = box(btnPtt, 0, 0, 2, 6, C_TEXT, 0);     lv_obj_align(micStem, LV_ALIGN_TOP_MID, 0, 42);
  micBase = box(btnPtt, 0, 0, 14, 2, C_TEXT, 1);    lv_obj_align(micBase, LV_ALIGN_TOP_MID, 0, 48);
  lblPtt = label(btnPtt, "HOLD TO TALK", F_SMALL, C_TEXT);
  lv_obj_align(lblPtt, LV_ALIGN_BOTTOM_MID, 0, -16);
  lv_obj_add_event_cb(btnPtt, ptt_event, LV_EVENT_ALL, nullptr);

  btnSend = make_button(pageDictate, bxS, by, bwS, bh);
  lblSendSym = label(btnSend, LV_SYMBOL_NEW_LINE, &lv_font_montserrat_14, C_TEXT);
  lv_obj_align(lblSendSym, LV_ALIGN_CENTER, 0, -14);
  lblSend = label(btnSend, "SEND", F_SMALL, C_TEXT);
  lv_obj_align(lblSend, LV_ALIGN_CENTER, 0, 10);
  lv_obj_add_event_cb(btnSend, send_event, LV_EVENT_ALL, nullptr);

  btnHf = make_button(pageDictate, bx2, by, bwH, bh);
  icoHf = icon(btnHf, &ic_handsfree, C_TEXT);
  lv_obj_align(icoHf, LV_ALIGN_CENTER, 0, -10);
  lblHf = label(btnHf, "", F_SMALL, C_TEXT);
  lv_obj_align(lblHf, LV_ALIGN_CENTER, 0, 22);
  lv_obj_add_event_cb(btnHf, hf_event, LV_EVENT_ALL, nullptr);

  // --- page 2: transforms (2 x 2 grid, table-driven) + utility strip ---
  {
    const int gx = 12, gy = 100, gw = 148, gh = 50, gap = 4;
    for (size_t i = 0; i < N_XFORMS && i < 4; i++) {
      lv_obj_t* b = make_button(pageXf, gx + (i % 2) * (gw + gap), gy + (i / 2) * (gh + gap), gw, gh);
      lv_obj_t* n = label(b, XFORMS[i].name, F_SMALL, C_TEXT); lv_obj_align(n, LV_ALIGN_CENTER, 0, -8);
      lv_obj_t* h = label(b, XFORMS[i].hint, &lv_font_montserrat_14, C_TEXT_2); lv_obj_align(h, LV_ALIGN_CENTER, 0, 11);
      lv_obj_add_event_cb(b, xf_event, LV_EVENT_CLICKED, (void*)&XFORMS[i]);
    }
    for (size_t i = N_XFORMS; i < 4; i++) {   // empty slot: dashed placeholder
      lv_obj_t* b = box(pageXf, gx + (i % 2) * (gw + gap), gy + (i / 2) * (gh + gap), gw, gh, C_BG, R_BTN);
      lv_obj_set_style_border_width(b, 1, 0); lv_obj_set_style_border_color(b, C(C_BORDER), 0);
      lv_obj_t* h = label(b, "empty slot", &lv_font_montserrat_14, C_TEXT_3); lv_obj_center(h);
    }
    const int sy = gy + 2 * gh + gap + 6, sh = 26;   // utility strip
    lv_obj_t* bA = make_button(pageXf, gx, sy, gw, sh);
    lv_obj_t* lA = label(bA, "Select all   Cmd A", &lv_font_montserrat_14, C_TEXT_2); lv_obj_center(lA);
    lv_obj_add_event_cb(bA, select_all_event, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* bD = make_button(pageXf, gx + gw + gap, sy, gw, sh);
    lv_obj_t* lD = label(bD, "View diff   Opt O", &lv_font_montserrat_14, C_TEXT_2); lv_obj_center(lD);
    lv_obj_add_event_cb(bD, diff_event, LV_EVENT_CLICKED, nullptr);
  }
  set_page(0);

  // --- toast ---
  toast = box(scr, 0, 0, 200, 30, C_SURFACE_HI, 15);
  lv_obj_align(toast, LV_ALIGN_TOP_MID, 0, 100);
  toastTxt = label(toast, "", F_SMALL, C_TEXT);
  lv_obj_center(toastTxt);
  lv_obj_add_flag(toast, LV_OBJ_FLAG_HIDDEN);

  render_state();
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
  Serial.println("Flow Ctrl v0.9");
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT); led(false, false, false);

  tft.init();
  tft.setRotation(PANEL_ROTATION);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
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
  BleKbd::begin(BLE_NAME);

  lv_timer_create(ui_tick, 50, nullptr);
  lv_timer_create(wave_tick, 70, nullptr);
  lastTick = millis();
}

void loop() {
  lv_tick_inc(millis() - lastTick);
  lastTick = millis();
  lv_timer_handler();
  delay(5);
}
