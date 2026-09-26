#!/usr/bin/env python3
"""Generates the game's pixel-art tiles and sprites (NOT part of the game at
runtime — run it to regenerate client/assets/*). Usage, from repo root:
    python3 tools/art/gen_tileset.py

Style reference: the CC0 "Tibia-style RPG Tileset" (Summer Engine, see
client/assets/CREDITS.md) — its palettes were sampled and are reused below.
That reference is one AI-generated picture, not usable tiles, so every tile
here is drawn from scratch on an exact 32x32 grid and wraps seamlessly.
Nothing is copied from Tibia/CipSoft assets.

Outputs (all in client/assets/):
  tilesets/terrain.png   32x32 ground tiles, one per cell (see TERRAIN order)
  tilesets/walls.png     32x48 wall pieces (top-only / top+front, 2 stones)
  tilesets/objects.png   64x64 cells: tree, bush, rock, flowers, ...
  sprites/player_<n>.png 32x48 frames: rows = down, left, right, up;
                         cols = idle, step A, step B
"""
import math
import os
import random

from PIL import Image, ImageDraw

T = 32
ROOT = os.path.join(os.path.dirname(__file__), "..", "..", "client", "assets")

# ------------------------------------------------------------------ palettes
# Sampled from the reference panels (dark -> light), widened slightly at both
# ends so pixel art has room for outlines and highlights.
GRASS = [(56, 104, 44), (78, 136, 52), (97, 158, 60), (110, 168, 64), (138, 188, 82), (176, 214, 110)]
GRASS_D = [(36, 78, 40), (48, 98, 48), (60, 116, 54), (70, 130, 58), (88, 148, 64), (118, 170, 78)]
DIRT = [(62, 42, 28), (83, 57, 38), (99, 70, 48), (112, 82, 55), (132, 100, 68), (158, 126, 90)]
SAND = [(150, 124, 80), (178, 150, 100), (200, 174, 122), (216, 194, 142), (232, 214, 166)]
STONE = [(40, 41, 46), (61, 63, 67), (84, 84, 87), (104, 102, 100), (122, 119, 114), (146, 142, 134), (168, 164, 154)]
STONE_D = [(24, 24, 30), (34, 34, 39), (46, 46, 51), (58, 58, 62), (70, 70, 72), (86, 86, 88), (104, 104, 104)]
WATER = [(34, 70, 120), (48, 92, 146), (60, 116, 170), (72, 132, 184), (104, 170, 212), (190, 226, 240)]
WOOD = [(64, 42, 30), (96, 65, 45), (122, 86, 58), (134, 95, 65), (150, 108, 74), (170, 128, 88)]
LEAF = [(26, 58, 36), (34, 76, 42), (48, 104, 50), (70, 132, 56), (96, 158, 62), (128, 184, 74)]
BARK = [(52, 34, 24), (80, 52, 34), (104, 70, 44), (126, 88, 56)]
OUTLINE = (24, 20, 22)
FLOWER = [(250, 244, 214), (236, 222, 170), (238, 196, 64)]


# ------------------------------------------------------------------ helpers
def h2(x, y, s):
    n = (x * 374761393 + y * 668265263 + s * 982451653) & 0xFFFFFFFF
    n = ((n ^ (n >> 13)) * 1274126177) & 0xFFFFFFFF
    return (n ^ (n >> 16)) / 0xFFFFFFFF


def pnoise(x, y, cell, s, period=T):
    """Value noise that repeats every `period` px — seamless tiles."""
    n = period // cell
    gx, gy = x / cell, y / cell
    x0, y0 = int(math.floor(gx)), int(math.floor(gy))
    fx, fy = gx - x0, gy - y0
    u, v = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)

    def g(i, j):
        return h2(i % n, j % n, s)
    a, b, c, d = g(x0, y0), g(x0 + 1, y0), g(x0, y0 + 1), g(x0 + 1, y0 + 1)
    return a + (b - a) * u + (c - a) * v + (a - b - c + d) * u * v


def bayer(x, y):
    m = ((0, 8, 2, 10), (12, 4, 14, 6), (3, 11, 1, 9), (15, 7, 13, 5))
    return (m[y % 4][x % 4] + 0.5) / 16.0


def pick(pal, v):
    return pal[max(0, min(len(pal) - 1, int(v * len(pal))))]


def new(w, h):
    return Image.new("RGBA", (w, h), (0, 0, 0, 0))


def put_wrapped(px, x, y, c):
    px[x % T, y % T] = c + (255,) if len(c) == 3 else c


def field(pal, lo, hi, seed, cell=8, dither=0.16):
    im = new(T, T)
    px = im.load()
    for y in range(T):
        for x in range(T):
            n = 0.65 * pnoise(x, y, cell, seed) + 0.35 * pnoise(x, y, 4, seed + 1)
            v = lo + (hi - lo) * n + (bayer(x, y) - 0.5) * dither
            px[x, y] = pick(pal, v) + (255,)
    return im


def scatter(seed, count, margin=0):
    r = random.Random(seed)
    return [(r.randrange(margin, T - margin), r.randrange(margin, T - margin)) for _ in range(count)]


# ------------------------------------------------------------------ terrain
def clover(px, x, y, pal_dark, pal_light):
    """3-leaf clump like the reference: light top-left, dark bottom-right."""
    for dx, dy in ((0, 0), (1, 0), (0, 1)):
        put_wrapped(px, x + dx, y + dy, pal_light)
    put_wrapped(px, x + 1, y + 1, pal_dark)
    put_wrapped(px, x + 2, y + 1, pal_dark)
    put_wrapped(px, x + 1, y + 2, pal_dark)


def grass(seed, flowers=0):
    im = field(GRASS, 0.30, 0.62, seed)
    px = im.load()
    for x, y in scatter(seed + 2, 22):
        clover(px, x, y, GRASS[1], GRASS[4])
    for x, y in scatter(seed + 3, 10):  # blades
        put_wrapped(px, x, y, GRASS[4])
        put_wrapped(px, x, y + 1, GRASS[2])
    for x, y in scatter(seed + 4, flowers):
        put_wrapped(px, x, y, FLOWER[0])
        put_wrapped(px, x + 1, y, FLOWER[1])
        put_wrapped(px, x, y + 1, FLOWER[1])
        put_wrapped(px, x + 1, y + 1, FLOWER[2])
        put_wrapped(px, x + 1, y + 2, GRASS[1])
    return im


def grass_dark(seed):
    im = field(GRASS_D, 0.20, 0.55, seed, cell=4)
    px = im.load()
    for x, y in scatter(seed + 2, 34):  # dense leafy clumps
        put_wrapped(px, x, y, GRASS_D[5])
        put_wrapped(px, x + 1, y, GRASS_D[4])
        put_wrapped(px, x, y + 1, GRASS_D[4])
        put_wrapped(px, x + 1, y + 1, GRASS_D[1])
        put_wrapped(px, x + 2, y + 1, GRASS_D[0])
    return im


def pebbles(px, seed, count, pal):
    r = random.Random(seed)
    for _ in range(count):
        x, y = r.randrange(T), r.randrange(T)
        w, h = r.choice(((2, 2), (3, 2), (3, 3), (4, 3)))
        for yy in range(h):
            for xx in range(w):
                edge = yy == h - 1 or xx == w - 1
                top = yy == 0 or xx == 0
                c = pal[1] if edge else (pal[5] if top else pal[4])
                put_wrapped(px, x + xx, y + yy, c)
        put_wrapped(px, x + w, y + h, pal[0])  # contact shadow


def dirt(seed):
    im = field(DIRT, 0.25, 0.62, seed)
    pebbles(im.load(), seed + 5, 7, DIRT)
    return im


def sand(seed):
    im = field(SAND, 0.30, 0.75, seed, dither=0.2)
    px = im.load()
    for x, y in scatter(seed + 6, 14):
        put_wrapped(px, x, y, SAND[4] if (x + y) % 2 else SAND[0])
    return im


def water(frame):
    im = field(WATER, 0.28, 0.55, 40, cell=16, dither=0.12)
    px = im.load()
    r = random.Random(41)
    crests = [(r.randrange(T), r.randrange(T), r.randrange(4, 8)) for _ in range(9)]
    for cx, cy, L in crests:
        cx += frame * 2  # waves drift right; 3 frames loop over 6 px, wraps
        for k in range(L):
            yy = cy - (1 if 0 < k < L - 1 else 0)
            put_wrapped(px, cx + k, yy, WATER[4])
            put_wrapped(px, cx + k, yy + 1, WATER[1])
        put_wrapped(px, cx + L // 2, cy - 2, WATER[5])
    return im


def wood_floor(seed):
    im = new(T, T)
    d = ImageDraw.Draw(im)
    for i, y in enumerate(range(0, T, 8)):
        d.rectangle([0, y, T - 1, y + 7], fill=WOOD[3 - (i % 2)])
        d.line([(0, y + 1), (T - 1, y + 1)], fill=WOOD[4])
        d.line([(0, y + 7), (T - 1, y + 7)], fill=WOOD[0])
        seam = (7 + 13 * i) % T
        d.line([(seam, y), (seam, y + 6)], fill=WOOD[0])
        d.point((seam + 1, y + 2), fill=WOOD[5])
    px = im.load()
    for x, y in scatter(seed, 20):  # grain
        if px[x, y][:3] not in (WOOD[0],):
            px[x, y] = WOOD[1] + (255,)
    return im


def stone_floor(seed, pal=STONE):
    im = new(T, T)
    d = ImageDraw.Draw(im)
    d.rectangle([0, 0, T - 1, T - 1], fill=pal[1])
    for (x0, y0, x1, y1) in ((0, 0, 15, 15), (16, 0, 31, 15), (8, 16, 23, 31), (-8, 16, 7, 31), (24, 16, 39, 31)):
        for (a, b, c, e) in ((x0, y0, x1, y1),):
            for xx in range(a, c + 1):
                for yy in range(b, e + 1):
                    col = pal[4]
                    if xx == a or yy == b:
                        col = pal[5]
                    if xx == c or yy == e:
                        col = pal[2]
                    put_wrapped(im.load(), xx, yy, col)
    px = im.load()
    for x, y in scatter(seed, 18):
        if px[x, y][:3] == pal[4]:
            px[x, y] = pal[3] + (255,)
    return im


# ------------------------------------------------------------------ walls
def wall_piece(pal, front, seed):
    """32x48 sprite for one wall tile, drawn so its bottom edge sits on the
    tile's bottom edge. The top face (16 px) is raised above the tile — the
    oblique look. `front`: draw the vertical face (only when the tile below
    isn't a wall; otherwise the next wall's top covers it)."""
    im = new(T, 48)
    d = ImageDraw.Draw(im)
    top_h = 32 if not front else 16
    # top slab(s)
    for i, x0 in enumerate((0, 16)):
        d.rectangle([x0, 0, x0 + 15, top_h - 1], fill=pal[5])
        d.line([(x0, 0), (x0 + 15, 0)], fill=pal[6])
        d.line([(x0, 0), (x0, top_h - 1)], fill=pal[6])
        d.line([(x0 + 15, 0), (x0 + 15, top_h - 1)], fill=pal[3])
        d.line([(x0, top_h - 1), (x0 + 15, top_h - 1)], fill=pal[3])
    px = im.load()
    for x, y in scatter(seed, 10):
        if y < top_h - 1 and px[x, y][:3] == pal[5]:
            px[x, y] = pal[4] + (255,)
    if front:
        d.rectangle([0, 16, T - 1, 47], fill=pal[3])
        for row, y in enumerate(range(16, 48, 8)):
            off = 0 if row % 2 else 8
            for x in range(-off, T, 16):
                a, b = max(0, x), min(T - 1, x + 15)
                d.rectangle([a, y, b, y + 7], fill=pal[3] if (x // 16 + row) % 2 else pal[2])
                d.line([(a, y), (b, y)], fill=pal[4])
                d.line([(b, y), (b, y + 7)], fill=pal[1])
                d.line([(a, y + 7), (b, y + 7)], fill=pal[1])
        d.line([(0, 16), (T - 1, 16)], fill=pal[0])  # crisp top edge shadow
    return im


# ------------------------------------------------------------------ objects
def selout(im):
    """Dark outline on the shadow side, softer tone on the lit side."""
    w, h = im.size
    src = im.load()
    out = new(w, h)
    o = out.load()
    for y in range(h):
        for x in range(w):
            if src[x, y][3]:
                continue
            for dx, dy in ((1, 0), (0, 1), (-1, 0), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h and src[nx, ny][3]:
                    b = src[nx, ny]
                    lit = dx > 0 or dy > 0
                    o[x, y] = (max(0, b[0] - 60), max(0, b[1] - 60), max(0, b[2] - 50), 255) if lit else OUTLINE + (255,)
                    break
    out.alpha_composite(im)
    return out


def blob(d, cx, cy, r, pal, seed, ragged=0.3):
    rnd = random.Random(seed)
    for y in range(int(cy - r) - 1, int(cy + r) + 2):
        for x in range(int(cx - r) - 1, int(cx + r) + 2):
            dx, dy = (x - cx) / r, (y - cy) / r
            dist = dx * dx + dy * dy
            if dist > 1 or (dist > 0.75 and rnd.random() < ragged):
                continue
            light = 0.62 - 0.42 * (dx + dy) / 1.4 - dist * 0.22 + (bayer(x, y) - 0.5) * 0.2
            d.point((x, y), fill=pick(pal, light))


def tree(seed):
    im = new(64, 64)
    d = ImageDraw.Draw(im)
    for y in range(46, 63):
        for x in range(28, 36):
            c = BARK[3] if x < 30 else (BARK[2] if x < 33 else BARK[1])
            if (x * 5 + y * 3) % 9 == 0:
                c = BARK[0]
            d.point((x, y), fill=c)
    d.point([(26, 62), (27, 61), (36, 61), (37, 62)], fill=BARK[1])
    r = random.Random(seed)
    clumps = [(32, 30, 17), (20, 30, 10), (44, 30, 10), (26, 18, 11), (40, 18, 11), (32, 11, 9), (32, 40, 12)]
    for i, (cx, cy, rad) in enumerate(sorted(clumps, key=lambda c: -c[1])):
        blob(d, cx + r.randrange(-1, 2), cy + r.randrange(-1, 2), rad, LEAF, seed * 13 + i)
    px = im.load()
    for _ in range(70):  # leaf texture: small light "V"s like the reference
        x, y = r.randrange(12, 52), r.randrange(4, 44)
        if px[x, y][3] and px[x, y][:3] in (LEAF[3], LEAF[4]):
            px[x, y] = LEAF[5] + (255,)
            if x + 1 < 64 and px[x + 1, y + 1][3]:
                px[x + 1, y + 1] = LEAF[2] + (255,)
    return selout(im)


def bush(seed):
    im = new(32, 32)
    d = ImageDraw.Draw(im)
    for i, (cx, cy, r) in enumerate(((11, 21, 8), (21, 20, 9), (16, 14, 8))):
        blob(d, cx, cy, r, LEAF, seed + i)
    d.point([(10, 17), (20, 13), (24, 21), (14, 23)], fill=(206, 64, 70))
    d.point([(10, 16), (20, 12)], fill=(250, 150, 150))
    return selout(im)


def rock(seed):
    im = new(32, 32)
    d = ImageDraw.Draw(im)
    blob(d, 15, 20, 9, STONE, seed, ragged=0.15)
    blob(d, 22, 23, 5, STONE, seed + 1, ragged=0.1)
    return selout(im)


def flower_patch(seed):
    im = new(32, 32)
    px = im.load()
    r = random.Random(seed)
    cols = [((240, 110, 140), (200, 70, 100)), ((250, 244, 214), (220, 200, 150)), ((150, 120, 220), (110, 80, 180))]
    for _ in range(7):
        x, y = r.randrange(4, 27), r.randrange(8, 28)
        c, cd = r.choice(cols)
        px[x, y + 2] = GRASS[1] + (255,)
        px[x, y + 3] = GRASS[1] + (255,)
        for dx, dy in ((0, 0), (1, 1), (-1, 1), (0, 2)):
            px[x + dx, y + dy - 1] = c + (255,)
        px[x, y] = FLOWER[2] + (255,)
        px[x + 1, y + 1] = cd + (255,)
    return im


def chest():
    im = new(32, 32)
    d = ImageDraw.Draw(im)
    d.rectangle([5, 14, 26, 27], fill=WOOD[3]); d.rectangle([5, 9, 26, 14], fill=WOOD[4])
    d.line([(5, 9), (26, 9)], fill=WOOD[5]); d.line([(5, 27), (26, 27)], fill=WOOD[0])
    d.rectangle([5, 14, 26, 15], fill=(140, 140, 150)); d.rectangle([14, 13, 17, 19], fill=(226, 190, 80))
    d.point((15, 17), fill=OUTLINE)
    return selout(im)


# ------------------------------------------------------------------ characters
def player_sheet(tunic, trousers, hair):
    """Rows: down, left, right, up. Cols: idle, step A, step B. 32x48 frames,
    feet on the frame bottom (sits on the tile's lower edge, head overlaps
    the tile above — like classic top-down MMOs)."""
    skin, skin_d = (236, 190, 152), (200, 146, 112)
    tl = tuple(min(255, v + 36) for v in tunic)
    td = tuple(max(0, v - 44) for v in tunic)
    trd = tuple(max(0, v - 30) for v in trousers)
    boot, boot_d = (74, 48, 32), (52, 32, 22)
    sheet = new(32 * 3, 48 * 4)

    def frame(direction, step):
        im = new(32, 48)
        d = ImageDraw.Draw(im)
        bob = 1 if step else 0
        la, ra = {0: (0, 0), 1: (-2, 2), 2: (2, -2)}[step]  # leg offsets
        side = direction in ("left", "right")
        # legs
        if side:
            d.rectangle([13 + la, 36 - bob, 16 + la, 44], fill=trousers)
            d.rectangle([16 + ra, 36 - bob, 19 + ra, 44], fill=trd)
            d.rectangle([12 + la, 44, 17 + la, 46], fill=boot)
            d.rectangle([15 + ra, 44, 20 + ra, 46], fill=boot_d)
        else:
            d.rectangle([12, 36 - bob, 15, 44 + min(0, la)], fill=trousers)
            d.rectangle([17, 36 - bob, 20, 44 + min(0, ra)], fill=trd)
            d.rectangle([11, 43 + min(0, la), 15, 46 + min(0, la)], fill=boot)
            d.rectangle([17, 43 + min(0, ra), 21, 46 + min(0, ra)], fill=boot_d)
        y0 = 22 - bob
        # torso
        if side:
            d.rectangle([12, y0, 20, y0 + 14], fill=tunic)
            d.rectangle([12, y0, 13, y0 + 14], fill=tl if direction == "right" else td)
            d.rectangle([19, y0, 20, y0 + 14], fill=td if direction == "right" else tl)
        else:
            d.rectangle([10, y0, 22, y0 + 14], fill=tunic)
            d.rectangle([10, y0, 12, y0 + 14], fill=tl)
            d.rectangle([20, y0, 22, y0 + 14], fill=td)
        d.rectangle([10 if not side else 12, y0 + 10, 22 if not side else 20, y0 + 11], fill=(96, 64, 38))
        if direction == "down":
            d.point((16, y0 + 10), fill=(230, 196, 90))
        # arms
        if side:
            fwd = 1 if direction == "right" else -1
            ax = 15 + fwd * (2 if step == 1 else (-2 if step == 2 else 0))
            d.rectangle([ax, y0 + 1, ax + 2, y0 + 10], fill=td)
            d.rectangle([ax, y0 + 10, ax + 2, y0 + 12], fill=skin_d)
        else:
            d.rectangle([7, y0 + 1 + (ra if step else 0) // 2, 9, y0 + 10], fill=tl)
            d.rectangle([23, y0 + 1 + (la if step else 0) // 2, 25, y0 + 10], fill=td)
            d.rectangle([7, y0 + 10, 9, y0 + 12], fill=skin)
            d.rectangle([23, y0 + 10, 25, y0 + 12], fill=skin_d)
        # head
        hy = y0 - 12
        if direction == "down":
            d.rectangle([12, hy + 2, 20, hy + 11], fill=skin)
            d.rectangle([18, hy + 4, 20, hy + 11], fill=skin_d)
            d.rectangle([11, hy, 21, hy + 4], fill=hair); d.rectangle([11, hy, 12, hy + 7], fill=hair)
            d.rectangle([20, hy, 21, hy + 6], fill=hair)
            d.point([(14, hy + 7), (18, hy + 7)], fill=(34, 26, 30))
            d.line([(15, hy + 9), (17, hy + 9)], fill=(176, 112, 92))
        elif direction == "up":
            d.rectangle([12, hy + 2, 20, hy + 11], fill=skin_d)
            d.rectangle([11, hy, 21, hy + 10], fill=hair)
            d.rectangle([12, hy + 1, 14, hy + 8], fill=tuple(min(255, v + 30) for v in hair))
        else:
            left = direction == "left"
            d.rectangle([13, hy + 2, 19, hy + 11], fill=skin)
            fx = 12 if left else 19
            d.rectangle([fx, hy + 5, fx + 1, hy + 8], fill=skin)  # nose side
            d.rectangle([12, hy, 20, hy + 4], fill=hair)
            hx = 17 if left else 12
            d.rectangle([hx, hy, hx + 3, hy + 8], fill=hair)
            ex = 14 if left else 17
            d.point((ex, hy + 6), fill=(34, 26, 30))
        return selout(im)

    for r, direction in enumerate(("down", "left", "right", "up")):
        for c in range(3):
            sheet.alpha_composite(frame(direction, c), (c * 32, r * 48))
    return sheet


# ------------------------------------------------------------------ output
TERRAIN = [  # order == atlas cell index (x = i % 8, y = i // 8); animation
    # frames must be consecutive in ONE row (Godot TileSet animation layout)
    ("grass", lambda: grass(1)),
    ("grass_b", lambda: grass(9)),
    ("grass_flowers", lambda: grass(3, flowers=6)),
    ("grass_dark", lambda: grass_dark(4)),
    ("dirt", lambda: dirt(5)),
    ("sand", lambda: sand(6)),
    ("wood_floor", lambda: wood_floor(7)),
    ("stone_floor", lambda: stone_floor(8)),
    ("water_0", lambda: water(0)),
    ("water_1", lambda: water(1)),
    ("water_2", lambda: water(2)),
    ("dungeon_floor", lambda: stone_floor(10, STONE_D)),
]
WALLS = [  # 32x48 cells, left to right
    ("wall_top", lambda: wall_piece(STONE, False, 1)),
    ("wall_front", lambda: wall_piece(STONE, True, 2)),
    ("dungeon_wall_top", lambda: wall_piece(STONE_D, False, 3)),
    ("dungeon_wall_front", lambda: wall_piece(STONE_D, True, 4)),
]
OBJECTS = [  # 64x64 cells, left to right; small sprites sit bottom-centre
    ("tree", lambda: tree(1)),
    ("tree_b", lambda: tree(2)),
    ("bush", lambda: bush(3)),
    ("rock", lambda: rock(4)),
    ("flowers", lambda: flower_patch(5)),
    ("chest", chest),
]
PLAYERS = {
    "player_blue": ((58, 100, 176), (78, 66, 80), (110, 70, 36)),
    "player_red": ((170, 52, 48), (70, 62, 58), (40, 34, 30)),
}


def main():
    os.makedirs(os.path.join(ROOT, "tilesets"), exist_ok=True)
    os.makedirs(os.path.join(ROOT, "sprites"), exist_ok=True)

    cols = 8
    rows = (len(TERRAIN) + cols - 1) // cols
    atlas = new(cols * T, rows * T)
    for i, (_, fn) in enumerate(TERRAIN):
        atlas.alpha_composite(fn(), ((i % cols) * T, (i // cols) * T))
    atlas.save(os.path.join(ROOT, "tilesets", "terrain.png"))

    walls = new(len(WALLS) * T, 48)
    for i, (_, fn) in enumerate(WALLS):
        walls.alpha_composite(fn(), (i * T, 0))
    walls.save(os.path.join(ROOT, "tilesets", "walls.png"))

    objs = new(len(OBJECTS) * 64, 64)
    for i, (_, fn) in enumerate(OBJECTS):
        spr = fn()
        objs.alpha_composite(spr, (i * 64 + (64 - spr.width) // 2, 64 - spr.height))
    objs.save(os.path.join(ROOT, "tilesets", "objects.png"))

    for name, (tunic, trousers, hair) in PLAYERS.items():
        player_sheet(tunic, trousers, hair).save(os.path.join(ROOT, "sprites", name + ".png"))

    print("terrain:", [n for n, _ in TERRAIN])
    print("walls:", [n for n, _ in WALLS])
    print("objects:", [n for n, _ in OBJECTS])


if __name__ == "__main__":
    main()
