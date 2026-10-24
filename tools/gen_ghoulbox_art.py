#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# FIENDBOX (a fork of GHOULBOX): the boot / ABOUT artwork as 4-bit alpha bitmaps (run by tools/build.py generate()):
#   GB_LOGO     "FiendBOX" in UnifrakturMaguntia (assets/fonts/UnifrakturMaguntia-Regular.ttf, SIL OFL 1.1), boot size
#   GB_LOGO_S   the same, ABOUT size (the height of the large UI face's line)
#   GB_IMP      an imp with a frost mark on its brow, drawn here at 4x and scaled down (smooth edges)
#   GB_SIGIL    a summoning circle: two rings, rune ticks, an eight-pointed star
#   GB_SKULL    the old name of the figure (GHOULBOX's skull): now the imp, so older code still builds
# Format as the icons: alpha 0..15, rows back to back, Huffman-coded with one code for all of them (GB_HC,
# gen_aa_font.py huff_pack); the firmware draws them with cv_alpha_hc in any colour.
#   gen_ghoulbox_art.py OUT.h [--preview PREVIEW.png]
import argparse
import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_aa_font import huff_pack, huff_unpack  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
FONT = ROOT / "assets/fonts/UnifrakturMaguntia-Regular.ttf"
WORD = "FiendBOX"        # (the lowercase x of this blackletter reads as an r, and all capitals blur: lowercase "iend", capital BOX)
SS = 4                   # supersampling of the drawn art


def logo(max_w, max_h):
    """the word as large as fits max_w x max_h, cropped to its ink"""
    size = max_h * 2
    while True:
        f = ImageFont.truetype(str(FONT), size)
        x0, y0, x1, y1 = f.getbbox(WORD)
        if x1 - x0 <= max_w and y1 - y0 <= max_h:
            break
        size -= 1
    im = Image.new("L", (x1 - x0 + 2, y1 - y0 + 2), 0)
    ImageDraw.Draw(im).text((1 - x0, 1 - y0), WORD, font=f, fill=255)
    return im.crop(im.getbbox())


def sigil(size):
    """a summoning circle: two rings, rune ticks between them, an eight-pointed star inside (dim lines)"""
    W = size * SS
    im = Image.new("L", (W, W), 0)
    d = ImageDraw.Draw(im)
    c = W / 2
    R = W / 2 - 2 * SS

    def ring(r, w, a=255):
        d.ellipse([c - r, c - r, c + r, c + r], outline=a, width=int(w * SS))
    ring(R, 1.6)
    ring(R * 0.86, 1.0)
    n = 36
    for k in range(n):                                  # the rune ticks: long and short, a glyph stroke every third
        a = 2 * math.pi * k / n
        r0, r1 = R * 0.88, R * (0.97 if k % 3 else 0.995)
        d.line([(c + r0 * math.cos(a), c + r0 * math.sin(a)), (c + r1 * math.cos(a), c + r1 * math.sin(a))],
               fill=200, width=int(0.9 * SS))
        if k % 3 == 0:
            b = a + math.pi / n * 0.9
            ra, rb = R * 0.90, R * 0.95
            d.line([(c + ra * math.cos(b), c + ra * math.sin(b)),
                    (c + rb * math.cos(a + math.pi / n * 1.5), c + rb * math.sin(a + math.pi / n * 1.5))],
                   fill=150, width=int(0.8 * SS))
    r = R * 0.84                                        # the star {8/3}, dim, behind the figure
    pts = [(c + r * math.cos(2 * math.pi * k / 8 - math.pi / 2), c + r * math.sin(2 * math.pi * k / 8 - math.pi / 2))
           for k in range(8)]
    for k in range(8):
        d.line([pts[k], pts[(k + 3) % 8]], fill=120, width=int(0.9 * SS))
    ring(R * 0.5, 0.8, 110)
    return im.resize((size, size), Image.LANCZOS)


def imp(w, h):
    """a horned imp: a bust in a ragged cloak, ink silhouette; eyes, a snowflake on the brow and the grin cut out"""
    W, H = w * SS, h * SS
    im = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(im)
    u = W / 64.0                                        # drawn on a 64 x 72 unit grid

    def P(pts):
        return [(x * u, y * u) for x, y in pts]
    for s in (1, -1):                                   # the horns, sweeping out and up
        def m(x, s=s):
            return 32 + s * (x - 32)
        d.polygon(P([(m(21), 18), (m(17), 8), (m(10), 2), (m(8), 0), (m(12), 8), (m(14), 15), (m(24), 23)]), fill=255)
    for s in (1, -1):                                   # the ears: long and pointed
        def m(x, s=s):
            return 32 + s * (x - 32)
        d.polygon(P([(m(16), 27), (m(2), 24), (m(0), 25), (m(5), 33), (m(17), 38)]), fill=255)
    d.ellipse([14 * u, 12 * u, 50 * u, 46 * u], fill=255)                              # the head
    d.polygon(P([(10, 72), (13, 56), (22, 47), (42, 47), (51, 56), (54, 72)]), fill=255)   # the cloak
    d.line([(56 * u, 72 * u), (60 * u, 62 * u), (57 * u, 53 * u)], fill=255, width=int(2.2 * u))   # the tail
    d.polygon(P([(57, 49), (61, 54), (57, 57), (53, 54)]), fill=255)                   # its spade
    for s in (1, -1):                                   # the eyes: angry slits
        def m(x, s=s):
            return 32 + s * (x - 32)
        d.polygon(P([(m(21), 26), (m(30), 29), (m(29), 33), (m(22), 31)]), fill=0)
    cx, cy = 32, 20                                     # the frost mark on the brow: six spokes
    for k in range(3):
        a = math.pi * k / 3 + math.pi / 6
        d.line([(cx * u - 4.2 * u * math.cos(a), cy * u - 4.2 * u * math.sin(a)),
                (cx * u + 4.2 * u * math.cos(a), cy * u + 4.2 * u * math.sin(a))], fill=0, width=max(1, int(1.1 * u)))
    d.ellipse([19 * u, 35 * u, 45 * u, 52 * u], fill=0)                                # the grin: a bowl cut out,
    d.ellipse([17 * u, 31 * u, 47 * u, 43 * u], fill=255)
    for x in (23.5, 28.5, 35.5, 40.5):                                                 # fangs hanging from its top edge
        d.polygon(P([(x - 2.0, 42), (x + 2.0, 42), (x, 47.5)]), fill=255)
    for x in (16, 26, 38, 48):                                                         # the cloak's ragged hem
        d.polygon(P([(x - 2.5, 72), (x, 66), (x + 2.5, 72)]), fill=0)
    return im.resize((w, h), Image.LANCZOS)


skull = imp          # (tools/gen_emu_art.py and older code ask for the figure by its GHOULBOX name)


def q4(im):
    return [min(15, (v + 8) // 17) for v in im.tobytes()]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--preview", help="write a 3x PNG of the boot screen in the FIEND colours")
    a = ap.parse_args()
    out = Path(a.out)
    arts = (("GB_LOGO", logo(232, 58)), ("GB_LOGO_S", logo(180, 34)), ("GB_IMP", imp(104, 117)), ("GB_SIGIL", sigil(208)))
    vals = [q4(im) for _, im in arts]
    hc, packed = huff_pack(vals)
    L = ["/* generated by tools/gen_ghoulbox_art.py: FIENDBOX's boot / ABOUT art, 4-bit alpha, Huffman-coded "
         "(cv_alpha_hc) */"]
    data = bytearray()
    for (name, im), v, b in zip(arts, vals, packed):
        assert huff_unpack(hc, b, len(v)) == v
        L += [f"#define {name}_W {im.size[0]}u", f"#define {name}_H {im.size[1]}u", f"#define {name} (GB_ART + {len(data)})"]
        print(f"fiendbox art: {name} {im.size[0]} x {im.size[1]}, {len(b)} B ({(len(v) + 1) // 2} B as nibbles)")
        data += b
    L += ["/* GHOULBOX's name for the figure: now the imp */", "#define GB_SKULL_W GB_IMP_W", "#define GB_SKULL_H GB_IMP_H",
          "#define GB_SKULL GB_IMP"]
    data += bytes(2)                                    # gfx.c cv_alpha_hc reads up to 2 bytes ahead
    for name, arr in (("GB_HC", hc), ("GB_ART", data)):
        rows = [", ".join(f"0x{b:02X}" for b in arr[i:i + 20]) for i in range(0, len(arr), 20)]
        L += [f"static const uint8_t {name}[] = {{"] + [f"    {r}," for r in rows] + ["};"]
    out.write_text("\n".join(L) + "\n")
    print(f"fiendbox art: {len(data) + len(hc)} B -> {out}")
    if a.preview:                                       # the boot screen as the splash will draw it (positions: ui_draw.c)
        BG, TEXT, THEME, ACC = (6, 8, 16), (214, 232, 240), (0, 200, 220), (255, 40, 90)
        scr = Image.new("RGB", (240, 240), BG)
        for (name, im), rgb, x, y in ((arts[3], ACC, 16, 0), (arts[2], TEXT, 68, 38), (arts[0], THEME, (240 - arts[0][1].size[0]) // 2, 150)):
            al = im.point(lambda v: min(15, (v + 8) // 17) * 17)
            scr.paste(Image.new("RGB", al.size, rgb), (x, y), al)
        scr.resize((720, 720), Image.NEAREST).save(a.preview)
        print(f"fiendbox art: preview -> {a.preview}")


if __name__ == "__main__":
    main()
