# Changelog

## v0.9.5 — 2026-09-16
- **Pairing recovery.** If the link fails at the encryption step twice (the computer's key no longer
  matches the board's — a flash with *erase* does this), the board drops its bonds and the status line
  says to remove and re-add it. **Forget pairing** on the About page (two taps) does the same on
  demand. The Flash page no longer offers to erase, so a normal reflash keeps the bond.
- Bench test rewritten where it was unclear: *Resync* now says what it is for; *Hands-free* says
  what Wispr does with the words while a session runs.

## v0.9.4 — 2026-09-16
- **Windows build** (`cyd2usb-win` env, `flow-ctrl-v0.9.4-cyd2usb-win.bin`): Alt/Ctrl names on screen,
  *Select all* sends `Ctrl+A` (the Mac build's `Cmd+A` becomes `Win+A` on Windows and opens the Action
  Center). Push-to-talk arrives as **Ctrl + Alt + T**. Second install button on the Flash page.
- **Boot splash:** the WOODWERD badge, the firmware version and build, and the push-to-talk chord the
  board is programmed for — four seconds, then a fade. Painted before anything else so there is no
  half-drawn frame at power-up.
- **About page:** long-press the *Flow Ctrl* title (version, build, chord, Bluetooth name and address,
  link state). Tap to close.
- **Discard is now useful after a hold:** it still cancels a running hands-free session (`Esc`), and
  otherwise sends Undo (`Cmd+Z` / `Ctrl+Z`) to remove the last paste. Previously Esc after a hold did
  nothing, because Wispr had already transcribed.
- `docs/TESTING.md`: the bench checklist a release has to pass.
- Build envs renamed `cyd2usb-mac` / `cyd2usb-win`; images `flow-ctrl-v0.9.4-cyd2usb-mac.bin` /
  `-win.bin`. Version lives in `include/version.h`; `tools/merge.sh` names the image from it.

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
