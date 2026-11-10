# Changelog

## v1.1.0 — 2026-11-09
- **Send now.** A square button to the right of **Send**, in landscape and portrait. It presses `Cmd+Return`
  on the Mac build and `Ctrl+Return` on the Windows build, and the screen says "Sent now". **Send** is
  narrower to make room and still presses a plain Return.
- The square shows the keys it presses over its name: the Command key and the return arrow on Mac, the
  word Ctrl and the return arrow on Windows. It dims while you hold to talk and waits for pairing, as Send does.
- New icon: the Command key (`tools/make_icons.py`).
- `tools/sim/make_guide.py` now writes `docs/screen-states-portrait.png` too, and finds a font on macOS.

## v1.0.0 — 2026-11-01
First stable release.

- **Portrait.** Long-press the title › About › **PORTRAIT** turns Flow Ctrl upright (240 × 320), for the
  portrait dock all the Ctrl boards share. Layout: one quiet status line on top; Hands-free and Discard
  with their icons; the big black pill with the words inside; full-width **Send** with a return-key icon.
  The Transform page is the two dots at the top right.
- **Flip.** About › **FLIP** turns either shape round so the USB cable can sit on the other side. Portrait
  starts with the cable at the bottom, as Media Ctrl does.
- The shape and side are remembered across power cycles and app-only updates (`0x10000`). A full `0x0`
  image resets to landscape. Switching redraws in place; the Bluetooth link stays up.
- Landscape is unchanged from v0.9.7. Its About page gains the two screen buttons; the hands-free help row
  made room for them, and the Bluetooth row now reads just "connected" / "pairing".
- New icons: trash and return key (`tools/make_icons.py` now writes them straight to C).
- The simulator renders both shapes (`p_*.ppm` alongside `s_*.ppm`); `docs/screen-states-portrait.png`.
- "Connected to Windows" on the Windows build (it said Mac).
- The Bluetooth library is pinned to NimBLE-Arduino 2.5.1, so a new build can't pick up a newer version by accident.
- A board on v0.9.7 can update from the Flash page and keep its Bluetooth pairing.

## Docs — 2026-11-01
- The docs site moves to WOODWERD Digital through the shared theme: the WOODWERD Toolkit strip with an All tools menu,
  a product header with no brand mark, the mark in the footer only, ink primary buttons, and Inter and JetBrains Mono
  bundled with the site instead of fetched from Google Fonts. The theme control now also follows the computer.

## Docs — 2026-10-29
- The docs site now wears the shared WOODWERD docs theme ([WOODWERD/ctrl-docs-theme](https://github.com/WOODWERD/ctrl-docs-theme)):
  WOODWERD Design mark, ink header with an amber rule, warm cream pages, Inter and JetBrains Mono, and a strip
  linking the Ctrl family. Docs build pinned to MkDocs 1.6 and Material 9.7.7.

## Docs — 2026-10-23
- The printed case moved to its own repository, [WOODWERD/cyd-case](https://github.com/WOODWERD/cyd-case), shared by every WOODWERD CYD
  firmware. Same housing v2 geometry; files renamed `CYD_Case_v2_*`. The build guide moved with it.

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
