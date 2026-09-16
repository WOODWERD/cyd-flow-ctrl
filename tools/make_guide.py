"""Annotated screens for docs/GETTING_STARTED.md. Reuses the PIL mock renderer.
    python3 tools/make_guide.py            → docs/guide-*.png"""
import os, sys
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mock
from mock import render, render_xf, W, H, C, font

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "docs")
os.makedirs(OUT, exist_ok=True)
SC = 2                      # screen scale in the guide
F_L, F_B, F_N, F_S = font(15, bold=False), font(16), font(14), font(13, bold=False)
BG, INK, MUTE, LINE, BADGE = "#F4F1EC", "#1A1A1D", "#6B6B70", "#B9B5AE", "#22C55E"

def annotate(frame, callouts, title, out, right_w=380):
    """callouts: list of (px, py, label, note) — px/py in 320x240 screen coords."""
    fw, fh = W * SC, H * SC
    pad = 28
    im = Image.new("RGB", (pad + fw + 40 + right_w + pad, pad + 40 + fh + pad), BG)
    d = ImageDraw.Draw(im)
    d.text((pad, pad - 6), title, font=font(20), fill=INK)
    fx, fy = pad, pad + 40
    # screen with a bezel
    d.rounded_rectangle((fx - 10, fy - 10, fx + fw + 10, fy + fh + 10), 22, fill="#0B0B0C")
    im.paste(frame.resize((fw, fh), Image.NEAREST), (fx, fy))
    # callouts
    lx = fx + fw + 40
    row_h = (fh) / max(len(callouts), 1)
    for i, (px, py, label, note) in enumerate(callouts):
        n = i + 1
        sx, sy = fx + px * SC, fy + py * SC
        ly = fy + row_h * i + 18
        d.ellipse((sx - 13, sy - 13, sx + 13, sy + 13), fill=BADGE, outline="#0B0B0C", width=2)
        t = d.textlength(str(n), font=F_B); d.text((sx - t / 2, sy - 10), str(n), font=F_B, fill="#08110A")
        d.ellipse((lx - 26, ly, lx - 2, ly + 24), fill=BADGE)
        t = d.textlength(str(n), font=F_B); d.text((lx - 14 - t / 2, ly + 2), str(n), font=F_B, fill="#08110A")
        d.text((lx + 6, ly + 2), label, font=F_B, fill=INK)
        # wrap the note
        words, lines, cur = note.split(), [], ""
        for w_ in words:
            test = (cur + " " + w_).strip()
            if d.textlength(test, font=F_L) > right_w - 40: lines.append(cur); cur = w_
            else: cur = test
        lines.append(cur)
        for j, ln in enumerate(lines[:3]):
            d.text((lx + 6, ly + 24 + j * 19), ln, font=F_L, fill=MUTE)
    im.save(os.path.join(OUT, out))
    print("wrote", out)

# --- Dictate page, at rest ---
annotate(render("idle"), [
    (60, 40,  "Listening indicator",  "Flat dots when idle. Bars move while Wispr is capturing — white for hold-to-talk, coral for hands-free."),
    (124, 40, "Discard",              "Sends Esc: throws away the current dictation without pasting."),
    (300, 40, "Page selector",        "A lozenge with a sliding green knob. Left: Dictate. Right: Transform. It only changes the page — it never sends anything."),
    (116, 134, "Hold to talk",         "Press and hold, speak, let go. Wispr transcribes and pastes at your cursor. Your everyday button."),
    (232, 134,"Send",                 "A Return key. Tap once the pasted words have landed to send a Claude prompt or a chat message."),
    (300, 134,"Hands-free",           "One tap locks Wispr into a session so you can talk for minutes without holding anything."),
], "Dictate page", "guide-dictate.png")

# --- Dictate page, hands-free running ---
annotate(render("hf"), [
    (60, 40,  "Session running",      "Coral bars and a coral rim mean Wispr is still listening. The status line counts the session up."),
    (116, 134, "Send moved left",      "Hold-to-talk slides away — it is meaningless mid-session — and Send takes its place."),
    (300, 134,"Stop",                 "Grown to fill the row. Tap to end the session; Wispr pastes everything you said. Long-press = reset the button only, if you stopped Flow from the Mac."),
], "Dictate page — hands-free running", "guide-handsfree.png")

# --- Transform page ---
annotate(render_xf(), [
    (152, 104, "Prompt Engineer",      "Wispr's built-in transform (Opt+2). Turns a rough spoken request into a well-structured prompt."),
    (304, 104,"Drawing note",         "Custom transform (Opt+3): terse construction-drawing callouts — material, size, finish, location."),
    (152, 158, "Client email",         "Custom transform (Opt+4): a warm, concise client email that keeps every number and date exact."),
    (152, 214, "Select all",           "Cmd+A. Transforms act on selected text, so this is the first tap when the text is in a prompt box."),
    (304, 214,"View diff",            "Opt+O. Shows what the transform changed."),
], "Transform page", "guide-transform.png")

# --- flows strip ---
def flows(rows, out):
    step_w, step_h, gap, pad = 190, 64, 30, 28
    n = max(len(r[1]) for r in rows)
    im = Image.new("RGB", (pad * 2 + n * step_w + (n - 1) * gap, pad + len(rows) * (step_h + 54) ), BG)
    d = ImageDraw.Draw(im)
    for r, (title, steps) in enumerate(rows):
        y = pad + r * (step_h + 54)
        d.text((pad, y - 4), title, font=F_B, fill=INK)
        y += 26
        for i, (big, small) in enumerate(steps):
            x = pad + i * (step_w + gap)
            d.rounded_rectangle((x, y, x + step_w, y + step_h), 14, fill="#1A1A1D")
            t = d.textlength(big, font=F_N); d.text((x + step_w / 2 - t / 2, y + 12), big, font=F_N, fill="#F5F5F4")
            t = d.textlength(small, font=F_S); d.text((x + step_w / 2 - t / 2, y + 36), small, font=F_S, fill="#8A8A90")
            if i < len(steps) - 1:
                ax = x + step_w + 6; ay = y + step_h / 2
                d.line((ax, ay, ax + gap - 12, ay), fill=LINE, width=2)
                d.polygon([(ax + gap - 12, ay - 6), (ax + gap - 4, ay), (ax + gap - 12, ay + 6)], fill=LINE)
    im.save(os.path.join(OUT, out)); print("wrote", out)

flows([
    ("Quick dictation", [("HOLD", "press and keep holding"), ("TALK", "a sentence or two"), ("RELEASE", "Wispr pastes"), ("SEND", "if it's a message")]),
    ("Long dictation (hands-free)", [("HANDS-FREE", "one tap"), ("TALK", "as long as you like"), ("STOP", "Wispr pastes it all"), ("SEND", "if it's a message")]),
    ("Prompt for Claude", [("HOLD & TALK", "rough idea into the box"), ("SELECT ALL", "Transform page"), ("PROMPT ENGINEER", "Wispr rewrites it"), ("SEND", "Dictate page")]),
], "guide-flows.png")


# --- exploded case, labelled ---
def exploded(src, out):
    base = Image.open(src).convert("RGB")
    # crop the empty margins of the Fusion render, then scale
    base = base.crop((300, 180, 1400, 1380)).resize((880, 960), Image.LANCZOS)
    pad = 28; right_w = 360
    im = Image.new("RGB", (pad + base.width + 30 + right_w + pad, pad + 40 + base.height + pad), BG)
    d = ImageDraw.Draw(im)
    d.text((pad, pad - 6), "What you are building", font=font(20), fill=INK)
    fx, fy = pad, pad + 40
    im.paste(base, (fx, fy))
    labels = [
        ((104, 70),  "Light pipe",  "Ø2.9 clear rod, 100 % infill. Pushes into the lid from inside; carries the light sensor through the lid."),
        ((640, 300), "Lid",         "Printed face-down. Four bosses take M3 × 4 mm heat-set inserts. The tongue over the USB-C port is part of it."),
        ((560, 520), "The board",   "ESP32-2432S028R Cheap Yellow Display, USB-C (2-USB) variant. Flash it before it goes in."),
        ((300, 780), "Base",        "Printed open-side-up. Four countersunk Ø3.2 holes underneath take M3 × 14 flat-head screws from below."),
    ]
    lx = fx + base.width + 30
    for i, ((px, py), label, note) in enumerate(labels):
        n = i + 1
        sx, sy = fx + px, fy + py
        d.ellipse((sx - 14, sy - 14, sx + 14, sy + 14), fill=BADGE, outline="#0B0B0C", width=2)
        t = d.textlength(str(n), font=F_B); d.text((sx - t / 2, sy - 10), str(n), font=F_B, fill="#08110A")
        ly = fy + i * 110 + 20
        d.ellipse((lx - 26, ly, lx - 2, ly + 24), fill=BADGE)
        t = d.textlength(str(n), font=F_B); d.text((lx - 14 - t / 2, ly + 2), str(n), font=F_B, fill="#08110A")
        d.text((lx + 6, ly + 2), label, font=F_B, fill=INK)
        words, lines, cur = note.split(), [], ""
        for w_ in words:
            test = (cur + " " + w_).strip()
            if d.textlength(test, font=F_L) > right_w - 40: lines.append(cur); cur = w_
            else: cur = test
        lines.append(cur)
        for j, ln in enumerate(lines[:4]):
            d.text((lx + 6, ly + 24 + j * 19), ln, font=F_L, fill=MUTE)
    y = fy + 4 * 110 + 30
    d.text((lx - 20, y), "Not shown: 4 × M3 × 14 countersunk screws (from below),", font=F_L, fill=MUTE)
    d.text((lx - 20, y + 19), "4 × M3 × 4 mm brass heat-set inserts (in the lid bosses).", font=F_L, fill=MUTE)
    im.save(os.path.join(OUT, out)); print("wrote", out)

exploded(os.path.join(OUT, "case_exploded.png"), "guide-exploded.png")

flows([
    ("Build order", [("BUY", "board, screws, inserts"), ("PRINT", "base, lid, light pipe"), ("INSERTS", "heat-set into the lid"), ("FLASH", "board on the bench"), ("ASSEMBLE", "board in, lid on, 4 screws")]),
], "guide-build.png")


# --- heat-set insert how-to: three panels ---
def inserts(out):
    pad, pw, ph, gap = 28, 300, 320, 24
    im = Image.new("RGB", (pad * 2 + 3 * pw + 2 * gap, pad + 30 + ph + 70), BG)
    d = ImageDraw.Draw(im)
    d.text((pad, pad - 6), "Heat-set inserts — four in the lid, before the board goes in", font=font(20), fill=INK)
    PET, BRASS, IRON, INK2 = "#C9C4BC", "#D4A64A", "#3A3A40", "#1A1A1D"
    caps = [("1  Rest", "Insert knurled-end down on the Ø4.0 boss hole. Iron at ~230 °C (PETG) / ~200 °C (PLA)."),
            ("2  Melt", "Touch the iron to the insert and let its weight sink it. Don't push. Keep it straight."),
            ("3  Flush", "Stop level with the boss face. Hold 5 s, lift straight off, let it cool before threading.")]
    for i, (title, cap) in enumerate(caps):
        x0 = pad + i * (pw + gap); y0 = pad + 30
        d.rounded_rectangle((x0, y0, x0 + pw, y0 + ph), 16, fill="#FFFFFF", outline=LINE)
        # boss section: a block with a Ø4 hole, 5 deep (scale 14 px/mm)
        S = 14; bx = x0 + pw / 2; base_y = y0 + ph - 24
        boss_w, boss_h, hole_w, hole_d = 6.2 * S, 8 * S, 4.0 * S, 5.0 * S
        d.rectangle((bx - boss_w / 2, base_y - boss_h, bx + boss_w / 2, base_y), fill=PET, outline=INK2)
        d.rectangle((bx - hole_w / 2, base_y - boss_h, bx + hole_w / 2, base_y - boss_h + hole_d), fill="#FFFFFF", outline=INK2)
        # insert (4 tall, ~4.1 OD) at different depths
        ins_h, ins_w = 4.0 * S, 4.1 * S
        depth = [ -ins_h, -ins_h * 0.45, 0 ][i]          # top of insert relative to boss face
        iy = base_y - boss_h + depth
        d.rectangle((bx - ins_w / 2, iy, bx + ins_w / 2, iy + ins_h), fill=BRASS, outline=INK2)
        for k in range(1, 4):                                   # knurl lines
            yy = iy + k * ins_h / 4; d.line((bx - ins_w / 2, yy, bx + ins_w / 2, yy), fill="#8A6A22", width=1)
        d.rectangle((bx - 1.5 * S, iy, bx + 1.5 * S, iy + ins_h), fill="#FFF3D6")  # bore
        # iron tip (panels 1,2)
        if i < 2:
            tip_y = iy - 4
            d.polygon([(bx - 9, tip_y - 46), (bx + 9, tip_y - 46), (bx + 3, tip_y), (bx - 3, tip_y)], fill=IRON)
            d.rectangle((bx - 13, tip_y - 96, bx + 13, tip_y - 46), fill="#6B6B70")
            if i == 1:
                for k in (-30, 30):   # heat waves beside the tip
                    d.arc((bx + k - 8, tip_y - 30, bx + k + 8, tip_y - 10), 200, 340, fill="#E0553A", width=2)
                    d.arc((bx + k - 8, tip_y - 16, bx + k + 8, tip_y + 4), 200, 340, fill="#E0553A", width=2)
        else:
            d.line((bx - boss_w / 2 - 30, base_y - boss_h, bx + boss_w / 2 + 30, base_y - boss_h), fill="#22C55E", width=2)
            d.text((bx + boss_w / 2 + 34, base_y - boss_h - 9), "flush", font=F_L, fill="#15803D")
        d.text((x0 + 14, y0 + 10), title, font=F_B, fill=INK2)
        # caption below panel
        words, lines, cur = cap.split(), [], ""
        for w_ in words:
            test = (cur + " " + w_).strip()
            if d.textlength(test, font=F_L) > pw - 10: lines.append(cur); cur = w_
            else: cur = test
        lines.append(cur)
        for j, ln in enumerate(lines[:3]):
            d.text((x0, y0 + ph + 10 + j * 19), ln, font=F_L, fill=MUTE)
    im.save(os.path.join(OUT, out)); print("wrote", out)

inserts("guide-inserts.png")
