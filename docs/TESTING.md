# Bench test — what a release has to pass

Ten minutes on the desk with the button paired to the Mac, Wispr Flow running, a text editor open.
Run it on the **release image** (the `.bin` from `release/`), not a dev build. Tick every line before
tagging a version; if one fails, it is not a release.

| # | Step | Pass looks like |
|---|---|---|
| 1 | **Boot.** Power-cycle the board. | WOODWERD badge appears at once with no half-drawn frame, then "Flow Ctrl · vX.Y.Z · Mac" and the chord (`Ctrl + Opt + T`). Fades after ~4 s to "Pairing…". |
| 2 | **Reconnect.** Do nothing. | Green dot within ~15 s. No re-pairing needed. If instead the status says "Remove Flow Ctrl on the computer, then add it again", do that once — the board found the Mac's key no longer matched — and note it. |
| 3 | **About.** Long-press the *Flow Ctrl* title. Then tap anywhere. | Page shows the version you flashed, the build, the chord, the address and "connected". The *Light sensor* value rises when you cover the light pipe and falls under a torch. Closes on tap. A short tap on the title does nothing. |
| 4 | **Hold.** Hold, say one sentence, release. Tap **SEND**. | Text pastes at the cursor after release. Return is sent. |
| 5 | **Undo.** Hold, say one sentence, release, wait for the paste, tap the **trash**. | The pasted sentence disappears. Toast says "Undo". |
| 6 | **Hands-free.** Tap the hand. Say two sentences with a long pause. Watch Wispr's bar collect the words (that is normal — nothing pastes until you stop). Tap the **black bar**. | The hand and dot turn red, red bars in the black bar, status line counts up "m:ss · TAP TO STOP". On the tap, one paste containing both sentences lands in the document and the screen goes back to idle. |
| 7 | **Cancel.** Tap the hand, say something, tap the **trash**. | Nothing pastes. Toast "Cancelled — nothing sent". Screen goes back to idle. |
| 8 | **Resync.** *Purpose: the board cannot see what Wispr does, so if you end a session from the Mac the button is left showing hands-free; this checks the way out.* Tap the hand, say a word, then press **Esc on the Mac's keyboard** so Wispr cancels. The board still shows the red hand — expected. Now **long-press the red hand** on the board. | The screen goes back to idle with the toast "Reset — nothing sent", and nothing extra is pasted. |
| 9 | **Transform.** Dictate into the editor. Slide to Transform. **Select all**, **Prompt Engineer**, **View diff**. Slide back. | Text highlights; Wispr rewrites it; the diff view opens. The hand, trash and page selector stay visible on both pages. |
| 10 | **Page safety.** On Dictate, hold with a heavy thumb near the top of HOLD TO TALK. | The page never changes. The lozenge only responds to a tap on itself. |
| 11 | **Link.** Leave it for 10 minutes. | Dot stays green. (Serial, if you are watching: no `disconnected` lines.) |
| 12 | **Cold link.** Turn Bluetooth off then on on the Mac. | Reconnects by itself within ~15 s. |

**Update keeps the pairing.** With the board paired, update it from the [Flash page](flash/index.md) (or flash
`-update.bin` at `0x10000`). Pass: after the reboot the dot goes green by itself — no removing and re-adding
Flow Ctrl on the computer.

**Windows image** (`-cyd2usb-win.bin`): steps 1–5 on a PC, with the chord set to `Ctrl + Alt + T`
and undo arriving as `Ctrl+Z`. Note anything the Windows Wispr app does differently on the Transform
page.

Record the version, date and any failure in the release notes.
