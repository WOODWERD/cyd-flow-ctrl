"""PIL mock of the Flow Ctrl screen (320x240) in its three states. Layout mirrors main.cpp.
Fonts: Inter if available, else Liberation Sans (metrics are close)."""
import random, glob, os
from PIL import Image, ImageDraw, ImageFont

def font(size, bold=True):
    for cand in ["Inter-SemiBold.ttf", "Inter-Bold.ttf", "LiberationSans-Bold.ttf" if bold else "LiberationSans-Regular.ttf"]:
        hits = glob.glob(f"/usr/share/fonts/**/{cand}", recursive=True)
        if hits: return ImageFont.truetype(hits[0], size)
    return ImageFont.load_default()

C = dict(BG="#0B0B0C", SURF="#1A1A1D", HI="#26262A", BORDER="#2E2E33", TEXT="#F5F5F4", T2="#8A8A90",
         T3="#55555B", WIDLE="#6E6E74", WLIVE="#FFFFFF", ACC="#FF5A36", ACCT="#14100E", OK="#34C759", WARN="#FFB020")
W, H = 320, 240
F14, F16, F14r = font(14), font(16), font(14, bold=False)

def topbar(d, connected, page):
    d.ellipse((16, 14, 24, 22), fill=C["OK"] if connected else C["WARN"])
    d.text((32, 9), "Flow Ctrl" if connected else "Pairing…", font=F14, fill=C["T2"])

def shared_row(im, d, state, page):
    rowY, rowH = 36, 56
    # waveform circle
    px, py, pw, ph = 12, rowY, rowH, rowH
    rim = C["ACC"] if state == "hf" else C["TEXT"] if state == "ptt" else C["BORDER"]
    d.ellipse((px, py, px + pw, py + ph), fill=C["SURF"], outline=rim)
    n, bw, gap = 7, 4, 2; span = n * bw + (n - 1) * gap
    random.seed(3)
    for i in range(n):
        if state in ("idle", None): h = 4; col = C["WIDLE"]
        else:
            env = 10 + 18 * (n // 2 - abs(i - n // 2)) // (n // 2); h = 6 + random.randrange(env)
            col = C["ACC"] if state == "hf" else C["WLIVE"]
        x = px + (pw - span) // 2 + i * (bw + gap); cy = py + ph // 2
        d.rounded_rectangle((x, cy - h // 2, x + bw, cy + h // 2), 2, fill=col)
    D = 56
    xTrash = px + D + 8; xSeg = xTrash + D + 8; segW = W - 12 - xSeg
    def icon(name, cx, cy, color):
        ic = Image.open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "icons", name + ".png"))
        tint = Image.new("RGBA", ic.size, color); tint.putalpha(ic.getchannel("A"))
        im.paste(tint, (int(cx - ic.width / 2), int(cy - ic.height / 2)), tint)
    # trash
    d.ellipse((xTrash, rowY, xTrash + D, rowY + D), fill=C["SURF"], outline=C["BORDER"])
    tc = C["T2"]; cxd, cyd = xTrash + D / 2, rowY + D / 2
    d.rectangle((cxd - 6, cyd - 4, cxd + 6, cyd + 8), outline=tc, width=2)
    d.line((cxd - 9, cyd - 6, cxd + 9, cyd - 6), fill=tc, width=2); d.line((cxd - 3, cyd - 9, cxd + 3, cyd - 9), fill=tc, width=2)
    d.line((cxd - 2, cyd - 1, cxd - 2, cyd + 5), fill=tc, width=1); d.line((cxd + 2, cyd - 1, cxd + 2, cyd + 5), fill=tc, width=1)
    # lozenge selector with sliding green knob
    ins = 4; half = (segW - 2 * ins) // 2
    d.rounded_rectangle((xSeg, rowY, xSeg + segW, rowY + D), 28, fill=C["SURF"], outline=C["BORDER"])
    kx = xSeg + ins + page * half
    d.rounded_rectangle((kx, rowY + ins, kx + half, rowY + D - ins), 24, fill=C["OK"])
    icon("ic_dictate", xSeg + ins + half / 2, rowY + D / 2, C["BG"] if page == 0 else C["T2"])
    icon("ic_transform", xSeg + ins + half * 1.5, rowY + D / 2, C["BG"] if page == 1 else C["T2"])

def render_xf(connected=True):
    im = Image.new("RGB", (W, H), C["BG"]); d = ImageDraw.Draw(im)
    topbar(d, connected, 1)
    shared_row(im, d, "idle", 1)
    gx, gy, gw, gh, gap = 12, 100, 148, 50, 4
    items = [("PROMPT ENGINEER", "Opt 2  ·  built in"), ("DRAWING NOTE", "Opt 3  ·  callouts"), ("CLIENT EMAIL", "Opt 4  ·  client tone"), None]
    for i, it in enumerate(items):
        x, y = gx + (i % 2) * (gw + gap), gy + (i // 2) * (gh + gap)
        if it is None:
            d.rounded_rectangle((x, y, x + gw, y + gh), 22, outline=C["BORDER"])
            t = d.textlength("empty slot", font=F14r); d.text((x + gw / 2 - t / 2, y + gh / 2 - 8), "empty slot", font=F14r, fill=C["T3"]); continue
        d.rounded_rectangle((x, y, x + gw, y + gh), 22, fill=C["SURF"], outline=C["BORDER"])
        t = d.textlength(it[0], font=F14); d.text((x + gw / 2 - t / 2, y + gh / 2 - 8 - 8), it[0], font=F14, fill=C["TEXT"])
        t = d.textlength(it[1], font=F14r); d.text((x + gw / 2 - t / 2, y + gh / 2 + 11 - 8), it[1], font=F14r, fill=C["T2"])
    sy, sh = gy + 2 * gh + gap + 6, 26
    for j, lab in enumerate(["Select all   Cmd A", "View diff   Opt O"]):
        x = gx + j * (gw + gap)
        d.rounded_rectangle((x, sy, x + gw, sy + sh), 13, fill=C["SURF"], outline=C["BORDER"])
        t = d.textlength(lab, font=F14r); d.text((x + gw / 2 - t / 2, sy + 5), lab, font=F14r, fill=C["T2"])
    return im

def render(state, connected=True, t="0:12"):
    im = Image.new("RGB", (W, H), C["BG"]); d = ImageDraw.Draw(im)
    topbar(d, connected, 0)
    shared_row(im, d, state, 0)
    status = {"idle": "Ready", "ptt": "Listening — release to paste", "hf": f"Hands-free  {t}"}[state]
    if not connected: status = 'Add "Flow Ctrl" in Bluetooth settings'
    tw = d.textlength(status, font=F14); d.text(((W - tw) / 2, 100), status, font=F14, fill=C["T2"])
    # buttons
    by, bh, bw2 = 128, H - 128 - 12, 112; bwS = 112; bwH = 64; bxS = 12 + bw2 + 4
    hf = state == "hf"
    if hf: bxS = 12; bxH, bwHH = 12 + bwS + 4, W - 12 - (12 + bwS + 4)
    else:  bxH, bwHH = bxS + bwS + 4, bwH
    # PTT (off-screen while hands-free)
    ptt = state == "ptt"
    if not hf:
        d.rounded_rectangle((12, by, 12 + bw2, by + bh), 22, fill=C["TEXT"] if ptt else C["SURF"], outline=C["BORDER"])
    mc = C["BG"] if ptt else C["TEXT"]; cx = 12 + bw2 // 2
    if not hf:
        d.rounded_rectangle((cx - 6, by + 16, cx + 6, by + 38), 6, fill=mc)
        d.arc((cx - 12, by + 22 - 6, cx + 12, by + 42), 0, 180, fill=mc, width=2)
        d.rectangle((cx - 1, by + 42, cx + 1, by + 48), fill=mc); d.rectangle((cx - 7, by + 48, cx + 7, by + 50), fill=mc)
        lab = "LISTENING" if ptt else "HOLD TO TALK"; tw = d.textlength(lab, font=F14)
        d.text((cx - tw / 2, by + bh - 16 - 16), lab, font=F14, fill=C["BG"] if ptt else C["TEXT"])
    # HF
    # SEND
    cxs = bxS + bwS // 2
    d.rounded_rectangle((bxS, by, bxS + bwS, by + bh), 22, fill=C["SURF"], outline=C["BORDER"])
    ay = by + bh / 2 - 14  # return arrow: down from top-right, then left with a head
    d.line((cxs + 7, ay - 8, cxs + 7, ay + 2), fill=C["TEXT"], width=2)
    d.line((cxs + 7, ay + 2, cxs - 8, ay + 2), fill=C["TEXT"], width=2)
    d.polygon([(cxs - 9, ay + 2), (cxs - 3, ay - 3), (cxs - 3, ay + 7)], fill=C["TEXT"])
    tw = d.textlength("SEND", font=F14); d.text((cxs - tw / 2, by + bh / 2 + 10 - 8), "SEND", font=F14, fill=C["TEXT"])
    bx2 = bxH; cx2 = bx2 + bwHH // 2
    d.rounded_rectangle((bx2, by, bx2 + bwHH, by + bh), 22, fill=C["ACC"] if hf else C["SURF"], outline=C["BORDER"])
    ic = Image.open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "icons", "ic_handsfree.png"))
    tint = Image.new("RGBA", ic.size, C["ACCT"] if hf else C["TEXT"]); tint.putalpha(ic.getchannel("A"))
    im.paste(tint, (int(cx2 - ic.width / 2), int(by + bh / 2 - (18 if hf else 10) - ic.height / 2)), tint)
    if hf:
        F20 = font(20); tw = d.textlength("STOP", font=F20); d.text((cx2 - tw / 2, by + bh / 2 + 18 - 11), "STOP", font=F20, fill=C["ACCT"])
    return im

if __name__ == "__main__":
    frames = [render("idle", connected=False), render("idle"), render("ptt"), render("hf"), render_xf()]
    sheet = Image.new("RGB", (W * 2 + 48, H * 3 + 64), "#222")
    for i, f in enumerate(frames):
        sheet.paste(f, (16 + (i % 2) * (W + 16), 16 + (i // 2) * (H + 16)))
    sheet = sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST)
    import os; sheet.save(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "flow-ctrl-mock.png"))
    print("ok")
