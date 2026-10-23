#!/usr/bin/env python3
"""Guide images for the docs, from screens rendered by the firmware itself (tools/sim).
Usage: python3 make_guide.py <dir with s_*.ppm> <docs dir>"""
import sys, os
from PIL import Image, ImageDraw, ImageFont
src, out = sys.argv[1], sys.argv[2]
BG = (244, 241, 236); INK = (17, 17, 17); SOFT = (107, 103, 96); BEZEL = (28, 28, 28); RED = (229, 72, 77)
FD = "/usr/share/fonts/truetype/dejavu/"
def font(sz, bold=False):
    for n in (("DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"),):
        try: return ImageFont.truetype(FD + n, sz)
        except OSError: pass
    return ImageFont.load_default()
F_NUM, F_TXT, F_HEAD, F_SMALL = font(22, True), font(21), font(22, True), font(17)
def screen(name, scale=2):
    im = Image.open(os.path.join(src, f"s_{name}.ppm")).convert("RGB")
    return im.resize((320 * scale, 240 * scale), Image.LANCZOS) if scale != 2 or True else im
def framed(name, scale=2, pad=10):
    s = screen(name).resize((320 * scale, 240 * scale), Image.LANCZOS)
    f = Image.new("RGB", (s.width + 2 * pad, s.height + 2 * pad), BG)
    d = ImageDraw.Draw(f); d.rounded_rectangle((0, 0, f.width - 1, f.height - 1), radius=18, fill=BEZEL)
    f.paste(s, (pad, pad)); return f
def badge(d, x, y, n):
    r = 17; d.ellipse((x - r, y - r, x + r, y + r), fill=INK, outline=(255, 255, 255), width=3)
    d.text((x, y), str(n), font=F_NUM, fill=(255, 255, 255), anchor="mm")
def guide(name, out_name, pts, items, title=None):
    c = Image.new("RGB", (1116, 576), BG); d = ImageDraw.Draw(c)
    fr = framed(name); ox, oy = 36, (576 - fr.height) // 2; c.paste(fr, (ox, oy))
    for n, (x, y) in enumerate(pts, 1): badge(d, ox + 10 + x * 2, oy + 10 + y * 2, n)
    tx, ty = 740, 70
    if title: d.text((tx, ty - 44), title, font=F_HEAD, fill=INK)
    for n, t in enumerate(items, 1):
        badge(d, tx + 2, ty + 12, n) if pts else None
        d.text((tx + 30 if pts else tx, ty), t, font=F_TXT, fill=INK); ty += 62 if len(items) < 7 else 52
    c.save(os.path.join(out, out_name)); print("wrote", out_name)

# Dictate: dot/title · hand · trash · selector · bar · send
guide("idle", "guide-dictate.png",
      [(8, 8), (132, 6), (186, 6), (262, 6), (40, 64), (40, 176)],
      ["Status · long-press for About", "Hands-free", "Discard (Esc / undo)", "Page selector", "Hold to talk", "Send"])
guide("hf", "guide-handsfree.png", [],
      ["Hand and dot turn red", "Red bars in the bar", "Time counts up", "Tap the bar to stop", "Trash = cancel (✕)", "Long-press hand = reset"],
      title="Hands-free running")
guide("xf", "guide-transform.png",
      [(10, 52), (160, 52), (10, 114), (10, 180), (160, 180)],
      ["Prompt Engineer · Opt 2", "Drawing note · Opt 3", "Client email · Opt 4", "Select all", "View diff"])

# Flows: three rows of small screens
rows = [("Quick", ["idle", "hold", "pasted", "sent"]),
        ("Long", ["idle", "hf", "pasted", "sent"]),
        ("Prompt", ["hold", "xf", "xf_pe", "sent"])]
c = Image.new("RGB", (906, 382), BG); d = ImageDraw.Draw(c)
tw, th, y = 160, 120, 4
for label, names in rows:
    d.text((10, y + th // 2), label, font=font(18, True), fill=INK, anchor="lm")
    x = 96
    for i, n in enumerate(names):
        t = screen(n).resize((tw, th), Image.LANCZOS)
        d.rounded_rectangle((x - 3, y - 3, x + tw + 2, y + th + 2), radius=8, fill=BEZEL); c.paste(t, (x, y))
        if i < len(names) - 1: d.text((x + tw + 14, y + th // 2), "→", font=font(22, True), fill=SOFT, anchor="mm")
        x += tw + 28
    y += th + 7
c.save(os.path.join(out, "guide-flows.png")); print("wrote guide-flows.png")

# All states, 2 x 3, for the firmware page
names = ["splash", "idle", "hold", "hf", "xf", "about"]
c = Image.new("RGB", (1376, 1568), (34, 34, 34))
for i, n in enumerate(names):
    c.paste(screen(n).resize((640, 480), Image.LANCZOS), (32 + (i % 2) * 672, 32 + (i // 2) * 512))
c.save(os.path.join(out, "screen-states.png")); print("wrote screen-states.png")
