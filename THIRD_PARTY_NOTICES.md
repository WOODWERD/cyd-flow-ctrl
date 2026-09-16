# Third-party notices

Flow Ctrl itself is © 2026 WOODWERD LLC, MIT-licensed (see `LICENSE`). It is built on, and distributes, the following.

## Compiled into the firmware (`release/*.bin` and the PlatformIO build)

| Component | License | Notes |
|---|---|---|
| [LVGL](https://github.com/lvgl/lvgl) 9.5 | MIT | UI toolkit. `include/lv_conf.h` is derived from LVGL's template. |
| [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) | FreeBSD (BSD-2) + portions BSD from Adafruit_GFX | Display driver |
| [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen) | MIT | Touch controller |
| [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) 2.x | Apache-2.0 (Apache NimBLE) | Bluetooth LE HID |
| [arduino-esp32](https://github.com/espressif/arduino-esp32) core | LGPL-2.1 | Arduino core for ESP32. Full source of this project is provided, satisfying relinking requirements. |
| [ESP-IDF](https://github.com/espressif/esp-idf) components (bundled by the platform) | Apache-2.0 | |
| [platform-espressif32](https://github.com/platformio/platform-espressif32) | Apache-2.0 | Build platform only; not distributed |

The BLE HID report map in `src/ble_kbd.cpp` follows the USB HID boot-keyboard layout published in the
USB HID Usage Tables (a public specification). The PnP ID values are the community-conventional ones
used by ESP32 BLE keyboard projects and do not claim a registered vendor ID.

## Fonts (`src/assets/*.c`, converted with `lv_font_conv`)

| Font | License | File |
|---|---|---|
| [Inter](https://github.com/rsms/inter) SemiBold 14/16/20 px | SIL Open Font License 1.1 | `src/assets/LICENSE-Inter-OFL.txt` |
| [IBM Plex Mono](https://github.com/IBM/plex) Medium 14 px | SIL Open Font License 1.1 | `src/assets/LICENSE-IBMPlex-OFL.txt` |

The `.c` files are bitmap conversions of subsets of these fonts, embedded in the firmware. Under the
OFL they are Modified Versions: they remain under the OFL, are not sold on their own, and the file
names use the original names only to identify their origin.

The UI icons (`src/assets/ic_*.c`, drawn by `tools/make_icons.py`) are original and MIT like the rest of the project.

## Hardware and CAD references

- The case was designed around the CYD board CAD model published in
  [witnessmenow/ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
  (`3dModels/Havenview_CYD_CAD`), which is MIT-licensed. That model is **not** redistributed here —
  only the case bodies designed for it. Board pin assignments come from the same repository's `PINS.md`.
- USB-C opening dimensions follow the public USB Type-C Cable and Connector Specification, Appendix B.
- "Cheap Yellow Display" / CYD is a community nickname for the Sunton ESP32-2432S028 board; no Sunton
  documentation is redistributed.

## Trademarks

**Wispr Flow** is a trademark of Wispr AI, Inc. This project is an independent, unofficial accessory
that emulates a Bluetooth keyboard and sends ordinary keyboard shortcuts to the Wispr Flow app. It is
not affiliated with, endorsed by, or supported by Wispr. No Wispr artwork, logos, or code are used;
the on-screen waveform pill is an original drawing inspired by the app's listening indicator.
"Flow Ctrl" is used descriptively (a key that triggers Flow) and is not claimed as a brand.
