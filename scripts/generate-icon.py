#!/usr/bin/env python3
"""WKjTX icon generator (v2).

Clean composition:
  - Dark rounded square base (#0a0907, flat).
  - Bold amber 'W' centered.
  - Three small amber arcs in the top-right corner, WiFi-signal style,
    evoking radio emission without overwhelming the letterform.
  - Subtle outer glow on the W for depth.

Writes multi-size ICO, Unix PNG, and preview PNGs.

Run from repo root:
    python scripts/generate-icon.py
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter, ImageFont

# -- palette ---------------------------------------------------------------
BG        = (10, 9, 7, 255)        # #0a0907
PANEL     = (20, 17, 13, 255)      # #14110d (subtle inner shade)
AMBER     = (255, 149, 0, 255)     # #ff9500
AMBER_HI  = (255, 179, 71, 255)    # #ffb347
AMBER_LO  = (196, 109, 0, 255)     # #c46d00
AMBER_GLOW = (255, 149, 0, 80)

# -- geometry --------------------------------------------------------------
CANVAS = 1024
CORNER_RADIUS_RATIO = 0.18


def rounded_square_mask(size, radius):
    m = Image.new("L", (size, size), 0)
    ImageDraw.Draw(m).rounded_rectangle(
        [(0, 0), (size - 1, size - 1)], radius=radius, fill=255)
    return m


def render_master():
    radius = int(CANVAS * CORNER_RADIUS_RATIO)

    # Base: solid dark rounded square.
    canvas = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    d = ImageDraw.Draw(canvas)
    d.rounded_rectangle([(0, 0), (CANVAS - 1, CANVAS - 1)],
                        radius=radius, fill=BG)

    # Inner 1px amber stroke (border accent).
    d.rounded_rectangle(
        [(4, 4), (CANVAS - 5, CANVAS - 5)],
        radius=radius - 4, outline=(AMBER_LO[0], AMBER_LO[1], AMBER_LO[2], 140),
        width=3)

    # -- signal waves in top-right corner (WiFi-like arcs) ----------------
    # All arcs share a virtual origin just outside the top-right corner of
    # the canvas so they look like a signal radiating OUT of that point.
    origin = (int(CANVAS * 0.82), int(CANVAS * 0.18))
    wave_layer = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    wd = ImageDraw.Draw(wave_layer)
    for r_px, stroke, col in [
        (int(CANVAS * 0.10), int(CANVAS * 0.015), AMBER_HI),
        (int(CANVAS * 0.17), int(CANVAS * 0.015), AMBER),
        (int(CANVAS * 0.24), int(CANVAS * 0.015), AMBER_LO),
    ]:
        wd.arc(
            [(origin[0] - r_px, origin[1] - r_px),
             (origin[0] + r_px, origin[1] + r_px)],
            start=135, end=225, fill=col, width=stroke)
    # small amber dot at the signal origin
    dot_r = int(CANVAS * 0.022)
    wd.ellipse(
        [(origin[0] - dot_r, origin[1] - dot_r),
         (origin[0] + dot_r, origin[1] + dot_r)],
        fill=AMBER_HI)

    # soft glow behind waves
    glow = wave_layer.filter(ImageFilter.GaussianBlur(radius=CANVAS * 0.01))
    canvas = Image.alpha_composite(canvas, glow)
    canvas = Image.alpha_composite(canvas, wave_layer)

    # -- central bold W ---------------------------------------------------
    # Font fallback list — sans-serif bold preferred.
    font = None
    for name, factor in [
        ("C:/Windows/Fonts/arialbd.ttf",  0.72),
        ("C:/Windows/Fonts/seguibl.ttf",  0.72),  # Segoe UI Black
        ("C:/Windows/Fonts/verdanab.ttf", 0.72),
        ("C:/Windows/Fonts/tahomabd.ttf", 0.72),
    ]:
        try:
            font = ImageFont.truetype(name, int(CANVAS * factor))
            break
        except Exception:
            continue
    if font is None:
        font = ImageFont.load_default()

    text = "W"
    tmp = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    td = ImageDraw.Draw(tmp)
    bbox = td.textbbox((0, 0), text, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    # Shift W slightly down-left to balance with the top-right waves.
    pos_x = (CANVAS - tw) // 2 - bbox[0] - int(CANVAS * 0.04)
    pos_y = (CANVAS - th) // 2 - bbox[1] + int(CANVAS * 0.03)

    # Glow behind W (wide, soft).
    glow_layer = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    ImageDraw.Draw(glow_layer).text((pos_x, pos_y), text, fill=AMBER_GLOW,
                                    font=font)
    glow_layer = glow_layer.filter(ImageFilter.GaussianBlur(radius=CANVAS * 0.035))
    canvas = Image.alpha_composite(canvas, glow_layer)

    # Main W in amber.
    main_layer = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    ImageDraw.Draw(main_layer).text((pos_x, pos_y), text, fill=AMBER,
                                    font=font)
    canvas = Image.alpha_composite(canvas, main_layer)

    # Upper highlight slice (lighter amber) — fake lighting from top.
    # Create hi_layer with the W in AMBER_HI, then CROP it to the upper
    # 32% of the glyph using Image.composite so the lower portion becomes
    # fully transparent (not semi-transparent — that would erode the
    # canvas underneath when alpha_composited).
    hi_layer = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    ImageDraw.Draw(hi_layer).text((pos_x, pos_y), text, fill=AMBER_HI,
                                  font=font)
    clip_y = pos_y + int(th * 0.32)
    crop_mask = Image.new("L", (CANVAS, CANVAS), 0)
    ImageDraw.Draw(crop_mask).rectangle(
        [(0, 0), (CANVAS, clip_y)], fill=255)
    blank = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    hi_cropped = Image.composite(hi_layer, blank, crop_mask)
    canvas = Image.alpha_composite(canvas, hi_cropped)

    # Final rounded-corner clip.
    final = Image.new("RGBA", (CANVAS, CANVAS), (0, 0, 0, 0))
    final.paste(canvas, (0, 0), rounded_square_mask(CANVAS, radius))

    return final


def main():
    repo_root = Path(__file__).resolve().parent.parent
    icons_win = repo_root / "jtdx-source" / "icons" / "windows-icons"
    icons_unix = repo_root / "jtdx-source" / "icons" / "Unix"
    preview_dir = repo_root / "jtdx-source" / "wkjtx" / "icons"

    for p in (icons_win, icons_unix, preview_dir):
        p.mkdir(parents=True, exist_ok=True)

    print("Rendering master 1024x1024...")
    master = render_master()

    master.resize((256, 256), Image.LANCZOS).save(preview_dir / "wkjtx_256.png")
    master.resize((256, 256), Image.LANCZOS).save(icons_unix / "wkjtx_icon.png")
    print(f"  wrote {icons_unix / 'wkjtx_icon.png'}")

    ico_sizes = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64),
                 (128, 128), (256, 256)]
    out_ico = icons_win / "wkjtx.ico"
    master.save(out_ico, format="ICO", sizes=ico_sizes)
    print(f"  wrote {out_ico} ({len(ico_sizes)} sizes)")

    for w, h in ico_sizes:
        master.resize((w, h), Image.LANCZOS).save(preview_dir / f"wkjtx_{w}.png")

    print("Done.")


if __name__ == "__main__":
    main()
