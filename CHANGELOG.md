# Changelog

## v0.9.7 — 2026-10-23
- **Light sensor reading** on the About page: the live value from the sensor under the lid's light
  pipe, with the lowest and highest seen since power-up. Groundwork for a backlight that follows the
  room; the backlight does not change with it yet.
- About page: hands-free row reads "tap the hand, tap bar to stop".

*(v0.9.6 was not released on its own; its changes ship in v0.9.7.)*

## v0.9.6 — 2026-10-23 (unreleased)
- **New look, matched to Wispr Flow.** Warm off-white screen, black type, and one red accent kept
  for live or destructive states. Build with `-DTHEME_DARK` for the previous dark colours.
- **The talk button is the waveform.** HOLD TO TALK is now one big black pill with the waveform
  inside (dots at rest, white bars while held, red bars during hands-free), like Wispr's Flow bar.
  A ring shows around it while you hold.
- **SEND** is one big button across the bottom.
- **Hands-free** is a red chip in the top bar, beside the trash. It turns solid red while a session
  runs; tap the talk pill (or the chip again) to stop and paste. The line under the pill shows the
  elapsed time, and the trash chip turns into a red ✕ (cancel).
- **Transform page:** 2 × 2 grid with Prompt Engineer in black, SELECT ALL and VIEW DIFF below.
- **About page:** a key/value table; FORGET PAIRING is a red outline that fills red when armed.
- **Splash:** WOODWERD badge in black, push-to-talk chord in red.
- **Updates keep the pairing.** The Flash page now writes the bootloader, partition table and app at
  their own addresses instead of one image from `0x0`, which used to blank the area holding the
  Bluetooth pairing — every update meant removing and re-adding Flow Ctrl on the computer. Releases
  also carry `…-update.bin` (app only, flash at `0x10000`) for esptool-js / ESP Ctrl.

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
