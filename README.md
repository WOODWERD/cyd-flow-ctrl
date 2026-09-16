# Flow Ctrl — a push-to-talk button for Wispr Flow on the Cheap Yellow Display

_An open-hardware project from [WOODWERD LLC](https://woodwerd.com), a design group in Austin, Texas._

A dedicated desk button for [Wispr Flow](https://wisprflow.ai) dictation, built on the
ESP32-2432S028R "Cheap Yellow Display" (CYD). The board pairs with your Mac as a **Bluetooth
keyboard** and sends the same keyboard shortcut you already use — so there is nothing to install on
the computer, and nothing about your Wispr setup changes.

- **Hold to talk** — press and hold the big button, speak, let go: Wispr transcribes and pastes.
- **Hands-free** — tap once to lock in, talk for as long as you like, tap **STOP** to paste it all.
- **Send** — a Return key, so a dictated Claude prompt or chat message goes without touching the keyboard.
- **Discard** — sends `Esc` to throw the dictation away.
- **Transforms** — a second page fires Wispr's AI rewrites (Prompt Engineer, plus your own) on selected text.
- Printed split case with only USB-C exposed. STL/STEP included.

**New here?** [Build it](docs/BUILD.md) (parts, printing, inserts, assembly) then the [Getting Started guide](docs/GETTING_STARTED.md) (setup and every button, with annotated screens).

![Dictate page](docs/guide-dictate.png)
![Case](docs/case_iso.png)

> Unofficial. Not affiliated with or endorsed by Wispr. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## How it works

Wispr Flow's push-to-talk is a keyboard shortcut. A MacBook's `Fn` key is not a real keycode, so no
external device can send it — but any chord with a normal modifier can be sent by a Bluetooth HID
keyboard, which is what the ESP32 pretends to be. The firmware ships with **Ctrl + Opt + T**; set
Wispr's *Push to talk* shortcut to that (Settings → General → Shortcuts), or rebuild with your own chord.

| On the screen | What the CYD sends | What Wispr does |
|---|---|---|
| **HOLD TO TALK** (press and hold) | chord held down for as long as your finger is | push-to-talk; release = transcribe & paste |
| **HANDS-FREE** (slashed-hand key, tap) | chord tapped twice, 120 ms apart | Wispr's "double-tap push-to-talk to lock" gesture → hands-free session |
| **STOP** (tap, while hands-free) | chord tapped once | ends the session, pastes |
| **STOP** (long-press) | nothing | resets the button's state only — use if you stopped Flow from the Mac |
| **SEND** | `Return` | sends what was just pasted — a Claude prompt, a chat message. Tap it once the words have landed |
| **Discard** (trash button in the top row) | `Esc` | discards the current dictation |
| **Page selector** (lozenge, top row) | nothing | slides between the Dictate and Transform pages |

There is no channel from Wispr back to the button, so the on-screen state is the button's best
guess. If they get out of sync, long-press STOP.

## Quick start (no toolchain)

1. **Flash.** Grab `release/flow-ctrl-v0.9-cyd2usb.bin` (USB-C / 2-USB board) or build the `cyd` env
   for the original micro-USB board. Either:
   - [ESP Web Tools / esptool-js](https://espressif.github.io/esptool-js/) in Chrome or Edge — 460800 baud, address `0x0`, or
   - `pip install esptool` then
     ```
     esptool.py --chip esp32 --port /dev/cu.usbserial-XXXX --baud 460800 write_flash 0x0 flow-ctrl-v0.9-cyd2usb.bin
     ```
   (Hold BOOT while plugging in if the port doesn't show up. 921600 baud is unreliable on the CH340.)
2. **Pair.** System Settings → Bluetooth → **"Flow Ctrl"**. The top-left dot turns green and the buttons
   enable. The bond is remembered and reconnects on power-up.
3. **Wispr Flow.** Settings → General → Shortcuts → *Push to talk* → press `Ctrl + Opt + T`.
   Wispr's hands-free lock follows the push-to-talk key automatically, so that's the only setting.
4. Talk.

If the chords do nothing in Terminal or a password field, that's macOS *Secure Keyboard Entry* — see
Wispr's help article on it. It affects every keyboard, not just this one.

## Page 2 — Transforms

The lozenge selector in the top row (talking head = Dictate, sparkles = Transform; a green knob slides to the active one) flips to a second page that fires Wispr's *Transforms* — AI rewrites
of whatever text is selected on the Mac. Buttons come from the `XFORMS` table at the top of `main.cpp`;
add a row, rebuild, and it appears in the next empty slot (four slots per page).

| Button | Sends | Set up in Wispr → Settings → Transforms |
|---|---|---|
| **PROMPT ENGINEER** | `Opt+2` | built in |
| **DRAWING NOTE** | `Opt+3` | custom: *Rewrite as terse construction-drawing callouts: material, size, finish, location. Fragments, not sentences. Uppercase. Keep dimensions exact.* |
| **CLIENT EMAIL** | `Opt+4` | custom: *Rewrite as a warm, concise client email from a residential designer. Plain language, no jargon. Keep every decision, dimension, price and date exact. End with one clear next step.* |
| **Select all** | `Cmd+A` | — |
| **View diff** | `Opt+O` | built in |

The Claude flow: dictate into the prompt box → **Select all** → **PROMPT ENGINEER** → back to **DICTATE** → **SEND**.
The top row — waveform circle, trash (Esc) and the page selector — is shared by both pages, so you can see a hands-free session running and discard it from either page. Switching is a tap, not a swipe, so a heavy finger on HOLD TO TALK can never turn into a page change.

## Docs

- [Build it](docs/BUILD.md) — bill of materials, which board to buy, printing, heat-set inserts, screws, assembly
- [Flash it](docs/flash/index.md) — one-click install from Chrome (on the docs site), or esptool
- [Getting Started](docs/GETTING_STARTED.md) — setup, every button and its intent, the three flows, troubleshooting
- [Case](case/README.md) — the printable files, print settings, tolerances
- [Firmware & code](docs/firmware.md) — build flags, code map, how to drive another app with it
- [Publishing checklist](docs/PUBLISHING.md) — how this reaches the CYD project list and the docs site

## Case

See [`case/README.md`](case/README.md): three parts, no supports, PETG, four M3 × 14 countersunk
screws and four M3 × 4 mm heat-set inserts. Only the USB-C port is exposed; a clear light pipe
carries the ambient-light sensor through the lid.

## License

Firmware, case and documentation: © 2026 WOODWERD LLC, released under MIT (see `LICENSE`). Third-party components and fonts retain their
own licences — listed in `THIRD_PARTY_NOTICES.md`.
