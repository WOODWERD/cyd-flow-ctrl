"""Draw the Flow Ctrl icons (talking head, sparkles, slashed hand) as 28 px alpha PNGs, then convert them
to LVGL A8 C arrays with LVGL's own LVGLImage.py. Run from the project root:
    python3 tools/make_icons.py .pio/libdeps/cyd2usb/lvgl/scripts/LVGLImage.py
Icons are alpha-only; the firmware recolors them per state."""
import sys, os, subprocess, math
from PIL import Image, ImageDraw

S = 28          # final size
K = 8           # supersample
N = S * K
OUT = os.path.join(os.path.dirname(__file__), "icons")
os.makedirs(OUT, exist_ok=True)

def canvas():
    im = Image.new("L", (N, N), 0)
    return im, ImageDraw.Draw(im)

def finish(im, name):
    im = im.resize((S, S), Image.LANCZOS)
    rgba = Image.new("RGBA", (S, S), (255, 255, 255, 0))
    rgba.putalpha(im)
    rgba.save(os.path.join(OUT, f"{name}.png"))

def P(x, y): return (x * K, y * K)

# --- Dictate: cropped face profile (forehead, nose, lips, chin) with speech dashes off the mouth ---
im, d = canvas()
face = [P(0, 0), P(8.5, 0), P(11.0, 2.5), P(11.8, 6.5),      # forehead / brow
        P(11.0, 8.5), P(13.2, 10.6), P(16.0, 13.4), P(13.2, 14.8),  # nose bridge, tip, under-nose
        P(13.0, 15.6), P(14.6, 16.6), P(12.8, 17.8),             # upper lip, mouth
        P(14.0, 18.8), P(13.2, 20.6), P(12.0, 22.6), P(9.8, 24.4), P(6.0, 25.2), P(0, 25.2)]   # lower lip, chin, jaw
d.polygon(face, fill=255)
# mouth notch (subtract) so the lips read
d.polygon([P(11.4, 16.2), P(15.6, 17.0), P(11.8, 18.2)], fill=0)
# speech: three dashes fanning out from the mouth, rounded ends
w = int(2.2 * K)
def dash(x0, y0, x1, y1):
    d.line((*P(x0, y0), *P(x1, y1)), fill=255, width=w)
    r = w / 2
    for (x, y) in ((x0, y0), (x1, y1)):
        cx, cy = P(x, y); d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=255)
dash(19.0, 17.0, 27.0, 17.0)        # straight out
dash(19.4, 13.6, 26.2, 10.8)        # up and out
dash(19.4, 20.4, 26.2, 23.2)        # down and out
finish(im, "ic_dictate")

# --- Transform: one big 4-point sparkle + two small ones (the AI-rewrite glyph) ---
def sparkle(d, cx, cy, r, pinch=0.28):
    pts = []
    for i in range(8):
        a = math.radians(i * 45 - 90)
        rr = r if i % 2 == 0 else r * pinch
        pts.append(P(cx + rr * math.cos(a), cy + rr * math.sin(a)))
    d.polygon(pts, fill=255)
im, d = canvas()
sparkle(d, 11.5, 15.5, 9.5)
sparkle(d, 21.5, 7.5, 4.2)
sparkle(d, 22.5, 20.5, 3.0)
finish(im, "ic_transform")


# --- Hands-free: open palm with four fingers and a thumb, a diagonal slash through it ---
im, d = canvas()
def capsule(x0, y0, x1, y1, r):
    d.rounded_rectangle((*P(x0, y0), *P(x1, y1)), radius=int(r * K), fill=255)
capsule(7.0, 12.0, 21.0, 25.5, 4.0)                      # palm
for i, (x, top) in enumerate([(7.4, 5.0), (11.0, 2.5), (14.6, 3.0), (18.2, 5.5)]):   # fingers
    capsule(x, top, x + 3.2, 15.0, 1.6)
capsule(2.5, 12.5, 8.5, 17.0, 1.8)                       # thumb (angled look via short capsule)
# slash: a dark gap then a bright stroke, corner to corner
w_gap, w_line = int(5.2 * K), int(2.4 * K)
d.line((*P(3.0, 3.0), *P(25.0, 25.0)), fill=0, width=w_gap)
d.line((*P(4.0, 4.0), *P(24.0, 24.0)), fill=255, width=w_line)
finish(im, "ic_handsfree")

# --- convert ---
if len(sys.argv) > 1:
    conv = sys.argv[1]
    cdir = os.path.join(os.path.dirname(__file__), "..", "src", "assets")
    for n in ("ic_dictate", "ic_transform", "ic_handsfree"):
        subprocess.check_call([sys.executable, conv, "--ofmt", "C", "--cf", "A8", "-o", cdir, os.path.join(OUT, f"{n}.png")])
    print("converted into", cdir)

# --- preview sheet ---
sheet = Image.new("RGB", (3 * 80, 80), "#1A1A1D")
for i, n in enumerate(("ic_dictate", "ic_transform", "ic_handsfree")):
    ic = Image.open(os.path.join(OUT, f"{n}.png")).resize((56, 56), Image.LANCZOS)
    tint = Image.new("RGBA", ic.size, (245, 245, 244, 255)); tint.putalpha(ic.getchannel("A"))
    sheet.paste(tint, (i * 80 + 12, 12), tint)
sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(os.path.join(OUT, "preview.png"))
print("ok")
