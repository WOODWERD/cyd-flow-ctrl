# Publishing checklist

_Status: housing v2 printed and test-fitted — works. One more housing revision planned before the public release; re-export `case/*.stl|step` and the renders from Fusion after it, then follow the steps below._

This folder is a ready-to-push GitHub repository. Steps, in order.

## 1. Create the repo and push
```
cd cyd-flow-ctrl
git init -b main
git add .
git commit -m "Flow Ctrl v0.9: firmware, case, docs"
gh repo create cyd-flow-ctrl --public --source=. --push     # or create it on github.com and push
```
Suggested repo description: *Push-to-talk / hands-free button for Wispr Flow on the ESP32 Cheap Yellow Display, with a printed case.*
Topics: `esp32`, `cheap-yellow-display`, `cyd`, `wispr-flow`, `ble-hid`, `lvgl`, `3d-printing`.

## 2. Make a release
Tag `v0.9` and attach `release/flow-ctrl-v0.9-cyd2usb.bin` (and a `cyd` build if you make one) so people
can flash without cloning. Keep the file in `release/` too — the README links to it.

## 3. Add it to the CYD project list
The CYD repo lists community projects in `PROJECTS.md` and asks that you add your own by PR at the
bottom of the table (open source + functional are the only requirements). Fork
`witnessmenow/ESP32-Cheap-Yellow-Display`, append this row, open the PR:

```
| Flow Ctrl | Push-to-talk / hands-free button for Wispr Flow dictation — the CYD pairs as a Bluetooth keyboard; includes a printed case | WOODWERD LLC | None | [Github](https://github.com/WOODWERD/cyd-flow-ctrl) | |
```

Adjust the column order to whatever the table has when you get there (Name · Description · Author ·
Additional Hardware? · Project Page · WebFlash). The case can also be offered as a line under
"Other cases from elsewhere" in `3dModels/README.md` pointing at `case/`, if you'd like it findable
from there too — that list already links out to Printables/MakerWorld/GitHub.

## 4. Optional: web flasher
The CYD list has a WebFlash column. A GitHub Pages site with ESP Web Tools and a `manifest.json`
pointing at the merged bin is an afternoon's work and lets people flash from Chrome with one click;
the Feeder-panel-style `esptool-js` route works but needs the user to type the address.

## 5. Optional: Printables / MakerWorld
Upload `case/*.stl` with the three renders in `docs/` and the text of `case/README.md`. Link back to
the repo for firmware. License choice there: CC-BY-4.0 is the usual pick for models (MIT is fine
too; just be consistent with the repo).

## Before pushing — things checked already
- No personal paths, serial-port names, or unrelated project references in the tree.
- Getting Started / Build guides and their images regenerate from `tools/make_guide.py` — rerun after any screen change so the pictures match the firmware.
- Fonts carry their OFL licence files; third-party libraries listed with licences.
- Wispr trademark disclaimer present; no Wispr artwork or code included.
- CYD board CAD (MIT) is referenced, not redistributed.
- BLE manufacturer string is "WOODWERD LLC", matching the LICENSE and README attribution.
