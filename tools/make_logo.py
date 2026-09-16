"""WOODWERD badge logo → 240 px wide LVGL A8 image (src/assets/logo_woodwerd.c) + a C header with the
same alpha bytes for the pre-LVGL boot paint. Run from the project root:
    python3 tools/make_logo.py .pio/libdeps/cyd2usb-mac/lvgl/scripts/LVGLImage.py
Source: tools/logo/Woodwerd_MAIN-LOGO.svg (© WOODWERD LLC)."""
import sys, os, subprocess, cairosvg
from PIL import Image
W = 240
here = os.path.dirname(__file__)
svg = os.path.join(here, "logo", "Woodwerd_MAIN-LOGO.svg")
png = os.path.join(here, "logo", "logo_woodwerd.png")
cairosvg.svg2png(url=svg, write_to=png, output_width=W * 4)   # 4× supersample, transparent bg
im = Image.open(png).convert("RGBA")
a = im.getchannel("A").resize((W, round(im.height * W / im.width)), Image.LANCZOS)
out = Image.new("RGBA", a.size, (255, 255, 255, 0)); out.putalpha(a); out.save(png)
print("logo", out.size)
if len(sys.argv) > 1:
    cdir = os.path.join(here, "..", "src", "assets")
    subprocess.check_call([sys.executable, sys.argv[1], "--ofmt", "C", "--cf", "A8", "-o", cdir, png])
    print("converted into", cdir)
