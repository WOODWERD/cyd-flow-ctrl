// Flow Ctrl — visual tokens. Cues taken from Wispr Flow's Flow Bar: a dark pill, white
// waveform bars that breathe while listening, quiet grey type, one accent for the
// "locked on" hands-free state. Swap the hex values here to retune the whole screen.
#pragma once
#include <lvgl.h>

#define C_BG          0x0B0B0C   // near-black canvas
#define C_SURFACE     0x1A1A1D   // pill / button rest state
#define C_SURFACE_HI  0x26262A   // pressed surface
#define C_BORDER      0x2E2E33
#define C_TEXT        0xF5F5F4
#define C_TEXT_2      0x8A8A90   // secondary / labels
#define C_TEXT_3      0x55555B   // hints
#define C_WAVE_IDLE   0x6E6E74
#define C_WAVE_LIVE   0xFFFFFF
#define C_ACCENT      0xFF5A36   // hands-free "locked on" (coral) — tune to Wispr's brand orange if desired
#define C_ACCENT_TEXT 0x14100E
#define C_OK          0x34C759   // connected dot
#define C_WARN        0xFFB020   // pairing dot

// Radii and metrics
#define R_PILL   28
#define R_BTN    22
#define R_BAR    2

// Fonts (generated with lv_font_conv, bpp 4)
#define F_TITLE   (&font_inter_20sb)
#define F_LABEL   (&font_inter_16sb)
#define F_SMALL   (&font_inter_14sb)
#define F_MONO    (&font_plex_14m)

static inline lv_color_t C(uint32_t hex) { return lv_color_hex(hex); }
