# Flow Ctrl on Omarchy

[Omarchy](https://omarchy.org) is a Linux desktop built on Hyprland. Wispr Flow has no Linux app, so on
Omarchy the button drives **Omarchy's own dictation** instead: whichever backend you picked in
*Setup › Defaults › Dictation* (Superwhisper, Voxtype or Hex). Transforms are rewritten by **Claude**,
using the same three prompts as the Mac build.

The board is the same, and it still pairs as a plain Bluetooth keyboard. What changes is a Linux
firmware build and a short setup script that teaches Hyprland what each chord means.

## 1 · Set up once

**Flash the Linux image.** Use the *Linux with Omarchy* button on the [Flash page](flash/index.md)
(Omarchy's Chromium can do it), or from a terminal:

```
pip install esptool
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash 0x0 flow-ctrl-v1.1.0-cyd2usb-linux.bin
```

The splash reads "Flow Ctrl · v1.1.0 · Linux".

**Pick a dictation backend**, if you haven't: Omarchy menu › Setup › Defaults › Dictation. Check it
works with Omarchy's own keys first (hold **Right Alt** and talk).

**Run the setup** from a clone of this repo:

```
git clone https://github.com/WOODWERD/cyd-flow-ctrl
cd cyd-flow-ctrl
./omarchy/install.sh
```

It installs two commands in `~/.local/bin`, the Hyprland binds (`~/.config/hypr/flow-ctrl.lua`,
loaded from your `bindings.lua`), and the Transform prompts in `~/.config/flow-ctrl/transforms/`.
It's safe to run again and never overwrites prompts or settings you've changed. Older Omarchy releases
that still use `hyprland.conf` get `flow-ctrl.conf` instead.

**Pair it.** Super + Ctrl + B (or the Bluetooth icon in the bar) › **Flow Ctrl**. The dot turns green.

**Optional — Transforms.** Add an Anthropic API key (from [console.anthropic.com](https://console.anthropic.com))
to `~/.config/flow-ctrl/env`:

```
ANTHROPIC_API_KEY=sk-ant-...
```

## 2 · What each button does on Omarchy

| On the screen | The board sends | Hyprland runs |
|---|---|---|
| **HOLD TO TALK** (hold) | `Ctrl + Alt + T`, held | `omarchy-dictation start`, then `stop` on release; the text types into the focused window |
| **Hands-free** (tap) | `Ctrl + Alt + H` | `omarchy-dictation toggle`, the same chord again to stop |
| **Stop** (tap the bar while hands-free) | `Ctrl + Alt + H` | stops and types it all |
| **Discard** while hands-free | `Ctrl + Alt + X` | `voxtype record cancel`; nothing is typed (see below for other backends) |
| **Discard** otherwise | `Ctrl + Z` | undo in the app in front |
| **SEND** / **Send now** | `Return` / `Ctrl + Return` | nothing; the keys go to the app |
| **PROMPT ENGINEER** · **DRAWING NOTE** · **CLIENT EMAIL** | `Ctrl + Alt + 2` · `3` · `4` | `flow-ctrl-transform`: copies the selection, rewrites it with Claude, pastes it over |
| **Select all** | `Ctrl + A` | nothing; the key goes to the app |
| **UNDO** (where the Mac has *View diff*) | `Ctrl + Z` | puts back the text a transform replaced |

The Claude flow works the same as on the Mac: dictate into the prompt box › **Select all** ›
**PROMPT ENGINEER** › back to **DICTATE** › **SEND**.

## 3 · Change the Transforms

Each prompt is a text file in `~/.config/flow-ctrl/transforms/`. The first line is its title, shown
while it works; the rest is the instruction Claude follows. Edit them freely. To point a button at a
different file, change its name in `~/.config/hypr/flow-ctrl.lua`.

Settings in `~/.config/flow-ctrl/env`:

- `FLOW_CTRL_MODEL` — default `claude-opus-5-5`.
- `FLOW_CTRL_EFFORT` — default `low`, which is quick and plenty for a rewrite. `medium` or `high` think harder.

The selected text is sent to Anthropic's API, and nothing else is.

## 4 · When something doesn't work

- **Holding the bar does nothing.** Run `flow-ctrl-dictate start` in a terminal, then `flow-ctrl-dictate stop`.
  If that types text, the binds aren't loaded: check that `~/.config/hypr/bindings.lua` ends with
  `require("hypr.flow-ctrl")` and run `hyprctl reload`. If it doesn't, fix dictation in Omarchy first.
- **Discard while hands-free still typed something.** Only Voxtype can cancel. With Superwhisper or Hex,
  Discard stops the recording and a notification says so; tap Discard again to undo what was typed.
- **A transform says "Nothing selected".** Select the text first (**Select all**, or with the mouse).
  In a terminal app the selection is the mouse highlight, and the result is pasted at the cursor
  rather than over the old text.
- **The on-screen state is wrong** (for example you stopped dictation with Omarchy's own keys):
  long-press the red hand to reset it, as on the Mac.
- Everything in [Getting started › When something does not work](GETTING_STARTED.md#6-when-something-does-not-work)
  about pairing applies here too.
