// Flow Ctrl — visual tokens. Cues taken from Wispr Flow: its Flow Bar's dark pill, white
// waveform bars that breathe while listening, quiet grey type, one accent for the
// "locked on" hands-free state. Swap the hex values here to retune the whole screen.
#pragma once
#include <lvgl.h>

// Default = light "paper" theme (v0.9.5): warm off-white canvas, black type, a black Flow-bar pill,
// and one red accent reserved for live/destructive states. Build with -DTHEME_DARK for the original look.
#ifndef THEME_DARK
#define C_BG          0xF7F4EE   // warm off-white canvas
#define C_SURFACE     0xECE7DE   // button rest state
#define C_SURFACE_HI  0xDFD8CB   // pressed surface
#define C_BORDER      0xD8D1C4
#define C_TEXT        0x111111   // ink
#define C_TEXT_2      0x6B6760   // secondary / labels
#define C_TEXT_3      0xA59F94   // hints
#define C_PILL        0x111111   // waveform pill (Wispr's Flow bar)
#define C_PILL_RIM    0x111111
#define C_WAVE_IDLE   0x5A5650
#define C_WAVE_LIVE   0xFFFFFF
#define C_ACCENT      0xE5484D   // hands-free / STOP / forget — red, live or destructive only
#define C_ACCENT_TEXT 0xFFFFFF
#define C_ACCENT_INK  0xC9353A   // red text on cream
#define C_ACCENT_TINT 0xFBE3E3   // red wash (trash chip during hands-free)
#define C_KNOB        0x111111   // page-selector knob
#define C_OK          0x2E9E5B   // connected dot
#define C_WARN        0xD98A00   // pairing dot (amber, readable on cream)
#else
#define C_BG          0x0B0B0C   // near-black canvas
#define C_SURFACE     0x1A1A1D   // pill / button rest state
#define C_SURFACE_HI  0x26262A   // pressed surface
#define C_BORDER      0x2E2E33
#define C_TEXT        0xF5F5F4
#define C_TEXT_2      0x8A8A90   // secondary / labels
#define C_TEXT_3      0x55555B   // hints
#define C_PILL        C_SURFACE
#define C_PILL_RIM    C_BORDER
#define C_WAVE_IDLE   0x6E6E74
#define C_WAVE_LIVE   0xFFFFFF
#define C_ACCENT      0xFF5A36   // hands-free "locked on" (coral)
#define C_ACCENT_TEXT 0x14100E
#define C_ACCENT_INK  C_ACCENT
#define C_ACCENT_TINT C_SURFACE_HI
#define C_KNOB        C_OK
#define C_OK          0x34C759   // connected dot
#define C_WARN        0xFFB020   // pairing dot
#endif

// RGB565 for drawing straight to the panel before LVGL starts
#define RGB565(h) ((uint16_t)((((h) >> 8) & 0xF800) | (((h) >> 5) & 0x07E0) | (((h) >> 3) & 0x001F)))

// Radii and metrics
#define R_PILL   28
#define R_BTN    22
#define R_BAR    2

// Fonts (generated with lv_font_conv, bpp 4)
#define F_TITLE   (&font_inter_20sb)
#define F_LABEL   (&font_inter_16sb)
#define F_SMALL   (&font_inter_14sb)
#define F_TINY    (&font_inter_12sb)   // status line under the talk pill
#define F_MONO    (&font_plex_14m)

static inline lv_color_t C(uint32_t hex) { return lv_color_hex(hex); }
