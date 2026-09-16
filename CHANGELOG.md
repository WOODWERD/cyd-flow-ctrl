# Changelog

## v0.9.3 — 2026-09-16
- **Fix:** the Bluetooth link no longer drops every minute or two on macOS. The Mac moves a bonded
  keyboard to a high-latency link; the ESP32 could sleep past a channel-map update and get
  disconnected with `0x228`. The firmware now asks for a latency-0 link once encrypted and keeps it.
- Serial log (115200) is timestamped, prints connection-parameter changes and decodes disconnect
  reasons.
- Default Bluetooth TX power (the +9 dBm boost gained nothing on the CYD's PCB antenna).
- Prebuilt image: `release/flow-ctrl-v0.9.3-cyd2usb.bin`.

## v0.9 — 2026-09-16
- First public release: Dictate page (Hold to talk / Send / Hands-free), Transform page, printed
  housing v2, docs site and one-click flasher.
