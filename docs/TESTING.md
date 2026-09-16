# Bench test — what a release has to pass

Ten minutes on the desk with the button paired to the Mac, Wispr Flow running, a text editor open.
Run it on the **release image** (the `.bin` from `release/`), not a dev build. Tick every line before
tagging a version; if one fails, it is not a release.

| # | Step | Pass looks like |
|---|---|---|
| 1 | **Boot.** Power-cycle the board. | WOODWERD badge appears at once with no half-drawn frame, then "Flow Ctrl · vX.Y.Z · Mac" and the chord (`Ctrl + Opt + T`). Fades after ~4 s to "Pairing…". |
| 2 | **Reconnect.** Do nothing. | Green dot within ~15 s. No re-pairing needed. |
| 3 | **About.** Long-press the *Flow Ctrl* title. Then tap anywhere. | Page shows the version you flashed, the build, the chord, the address and "connected". Closes on tap. A short tap on the title does nothing. |
| 4 | **Hold.** Hold, say one sentence, release. Tap **SEND**. | Text pastes at the cursor after release. Return is sent. |
| 5 | **Undo.** Hold, say one sentence, release, wait for the paste, tap the **trash**. | The pasted sentence disappears. Toast says "Undo". |
| 6 | **Hands-free.** Tap the hand. Say two sentences with a long pause. Tap **STOP**. | HOLD slides off, coral STOP fills the row, status counts up. One paste containing both sentences. Row slides back. |
| 7 | **Cancel.** Tap the hand, say something, tap the **trash**. | Nothing pastes. Toast "Cancelled — nothing sent". Row resets. |
| 8 | **Resync.** Tap the hand, then stop Wispr from the Mac (Esc on the keyboard). Long-press **STOP**. | Row resets, toast "Reset — nothing sent", nothing extra pasted. |
| 9 | **Transform.** Dictate into the editor. Slide to Transform. **Select all**, **Prompt Engineer**, **View diff**. Slide back. | Text highlights; Wispr rewrites it; the diff view opens. The waveform circle and trash stay visible on both pages. |
| 10 | **Page safety.** On Dictate, hold with a heavy thumb near the top of HOLD TO TALK. | The page never changes. The lozenge only responds to a tap on itself. |
| 11 | **Link.** Leave it for 10 minutes. | Dot stays green. (Serial, if you are watching: no `disconnected` lines.) |
| 12 | **Cold link.** Turn Bluetooth off then on on the Mac. | Reconnects by itself within ~15 s. |

**Windows image** (`-cyd2usb-win.bin`): steps 1–5 on a PC, with the chord set to `Ctrl + Alt + T`
and undo arriving as `Ctrl+Z`. Note anything the Windows Wispr app does differently on the Transform
page.

Record the version, date and any failure in the release notes.
