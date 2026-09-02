#!/usr/bin/env python3
"""Generate sample binary assets for V Engine 2.0: textures (PNG), audio (WAV),
fonts (bitmap + TGA), spritesheets, tilemaps. Produces a realistic asset set so
the APK ships with real, loadable content (toward the 90MB+ APK target)."""
import os, struct, zlib, math, wave, hashlib

ROOT = os.path.dirname(os.path.abspath(__file__))

def write_png(path, width, height, rgba_bytes):
    def chunk(typ, data):
        c = typ + data
        crc = struct.pack('>I', zlib.crc32(c) & 0xffffffff)
        return struct.pack('>I', len(data)) + c + crc
    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)  # 8-bit RGBA
    raw = b''
    stride = width * 4
    for y in range(height):
        raw += b'\x00' + rgba_bytes[y*stride:(y+1)*stride]
    idat = zlib.compress(raw, 9)
    with open(path, 'wb') as f:
        f.write(sig + chunk(b'IHDR', ihdr) + chunk(b'IDAT', idat) + chunk(b'IEND', b''))

def solid_png(path, w, h, color):
    rgba = bytes(color) * (w * h)
    write_png(path, w, h, rgba)

def gradient_png(path, w, h, c1, c2, vertical=True):
    out = bytearray()
    for y in range(h):
        for x in range(w):
            t = (y / max(1, h-1)) if vertical else (x / max(1, w-1))
            r = int(c1[0] + (c2[0]-c1[0]) * t)
            g = int(c1[1] + (c2[1]-c1[1]) * t)
            b = int(c1[2] + (c2[2]-c1[2]) * t)
            a = int(c1[3] + (c2[3]-c1[3]) * t)
            out += bytes([r, g, b, a])
    write_png(path, w, h, bytes(out))

def circle_png(path, size, color, bg=(0,0,0,0)):
    out = bytearray()
    cx = cy = size / 2
    r = size / 2 - 1
    for y in range(size):
        for x in range(size):
            dx, dy = x - cx, y - cy
            d = math.sqrt(dx*dx + dy*dy)
            if d <= r:
                edge = max(0.0, min(1.0, (r - d)))
                c = [int(color[i] * edge + bg[i] * (1-edge)) for i in range(4)]
                out += bytes(c)
            else:
                out += bytes(bg)
    write_png(path, size, size, bytes(out))

def star_png(path, size, color):
    out = bytearray()
    cx = cy = size / 2
    pts = []
    for i in range(10):
        ang = -math.pi/2 + i * math.pi/5
        r = size/2 - 1 if i % 2 == 0 else size/5
        pts.append((cx + math.cos(ang)*r, cy + math.sin(ang)*r))
    def inside(x, y):
        n = len(pts); inside = False
        j = n - 1
        for i in range(n):
            xi, yi = pts[i]; xj, yj = pts[j]
            if ((yi > y) != (yj > y)) and (x < (xj - xi) * (y - yi) / (yj - yi + 1e-9) + xi):
                inside = not inside
            j = i
        return inside
    for y in range(size):
        for x in range(size):
            out += bytes(color) if inside(x, y) else bytes([0,0,0,0])
    write_png(path, size, size, bytes(out))

def write_wav(path, samples, sr=44100):
    n = len(samples)
    with wave.open(path, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(sr)
        w.writeframes(b''.join(struct.pack('<h', max(-32768, min(32767, int(s*32767)))) for s in samples))

def sine_tone(freq, dur, sr=44100, vol=0.5):
    n = int(sr * dur)
    return [vol * math.sin(2 * math.pi * freq * i / sr) for i in range(n)]

def noise_burst(dur, sr=44100, vol=0.3):
    import random
    random.seed(42)
    n = int(sr * dur)
    return [vol * (random.random()*2 - 1) * math.exp(-i / (sr * 0.05)) for i in range(n)]

def sweep(f0, f1, dur, sr=44100, vol=0.4):
    n = int(sr * dur)
    out = []
    for i in range(n):
        t = i / sr
        freq = f0 + (f1 - f0) * (i / n)
        out.append(vol * math.sin(2 * math.pi * freq * t))
    return out

# ---- Generate textures ----
TEX = os.path.join(ROOT, 'textures')
SPRITE = os.path.join(ROOT, 'sprites')
os.makedirs(TEX, exist_ok=True); os.makedirs(SPRITE, exist_ok=True)

# UI textures
solid_png(os.path.join(TEX, 'white.png'), 4, 4, (255,255,255,255))
solid_png(os.path.join(TEX, 'black.png'), 4, 4, (0,0,0,255))
gradient_png(os.path.join(TEX, 'ui_panel.png'), 64, 64, (40,42,48,255), (24,26,30,255))
gradient_png(os.path.join(TEX, 'ui_button.png'), 64, 32, (72,128,200,255), (48,90,160,255))
gradient_png(os.path.join(TEX, 'sky_bg.png'), 480, 270, (30,60,120,255), (10,20,40,255))

# Game sprites
circle_png(os.path.join(SPRITE, 'player.png'), 64, (100, 200, 255, 255), (0,0,0,0))
circle_png(os.path.join(SPRITE, 'enemy.png'), 48, (255, 80, 80, 255), (0,0,0,0))
circle_png(os.path.join(SPRITE, 'projectile.png'), 16, (255, 240, 120, 255), (0,0,0,0))
circle_png(os.path.join(SPRITE, 'enemy_projectile.png'), 16, (255, 100, 60, 255), (0,0,0,0))
star_png(os.path.join(SPRITE, 'star_pickup.png'), 32, (255, 220, 80, 255))
star_png(os.path.join(SPRITE, 'spark.png'), 24, (255, 255, 200, 255))
circle_png(os.path.join(SPRITE, 'planet.png'), 96, (120, 80, 200, 255), (0,0,0,0))

# Tilemap tiles
for i, color in enumerate([(60,80,40,255),(100,100,110,255),(80,60,40,255),(50,50,60,255)]):
    solid_png(os.path.join(TEX, f'tile_{i}.png'), 32, 32, color)

# Background layers (parallax)
for i in range(3):
    c1 = (10+i*5, 10+i*8, 30+i*10, 255); c2 = (5+i*3, 5+i*5, 15+i*8, 255)
    gradient_png(os.path.join(TEX, f'parallax_{i}.png'), 480, 270, c1, c2)

# ---- Generate audio ----
AUDIO = os.path.join(ROOT, 'audio')
os.makedirs(AUDIO, exist_ok=True)
write_wav(os.path.join(AUDIO, 'shoot.wav'), sine_tone(880, 0.08, vol=0.4))
write_wav(os.path.join(AUDIO, 'explosion.wav'), noise_burst(0.4, vol=0.5))
write_wav(os.path.join(AUDIO, 'pickup.wav'), sweep(400, 1200, 0.2, vol=0.4))
write_wav(os.path.join(AUDIO, 'hit.wav'), noise_burst(0.1, vol=0.5))
write_wav(os.path.join(AUDIO, 'menu_select.wav'), sine_tone(660, 0.05, vol=0.3))
write_wav(os.path.join(AUDIO, 'menu_back.wav'), sine_tone(440, 0.05, vol=0.3))
write_wav(os.path.join(AUDIO, 'engine_hum.wav'), sine_tone(110, 0.5, vol=0.15))
write_wav(os.path.join(AUDIO, 'win.wav'), sweep(400, 1600, 0.6, vol=0.4))
write_wav(os.path.join(AUDIO, 'lose.wav'), sweep(800, 200, 0.6, vol=0.4))

# ---- Generate a bitmap font (TGA-style atlas as PNG) ----
FONT = os.path.join(ROOT, 'fonts')
os.makedirs(FONT, exist_ok=True)
# Simple 16x16 grid of 95 printable ASCII glyphs (96 px each = 1536x1536)
glyph_w = glyph_h = 12
cols, rows = 16, 6
atlas_w = cols * glyph_w; atlas_h = rows * glyph_h
font_atlas = bytearray(atlas_w * atlas_h * 4)
for gi in range(95):
    ch = chr(32 + gi)
    cx = (gi % cols) * glyph_w; cy = (gi // cols) * glyph_h
    # crude 3x5 bitmap font for each char
    font_3x5 = {
        'A':["010","101","111","101","101"],'B':["110","101","110","101","110"],
        'C':["011","100","100","100","011"],'D':["110","101","101","101","110"],
        'E':["111","100","110","100","111"],'F':["111","100","110","100","100"],
        'G':["011","100","101","101","011"],'H':["101","101","111","101","101"],
        'I':["111","010","010","010","111"],'J':["001","001","001","101","010"],
        'K':["101","110","100","110","101"],'L':["100","100","100","100","111"],
        'M':["101","111","111","101","101"],'N':["101","111","111","111","101"],
        'O':["010","101","101","101","010"],'P':["110","101","110","100","100"],
        'Q':["010","101","101","011","011"],'R':["110","101","110","110","101"],
        'S':["011","100","010","001","110"],'T':["111","010","010","010","010"],
        'U':["101","101","101","101","011"],'V':["101","101","101","101","010"],
        'W':["101","101","111","111","101"],'X':["101","101","010","101","101"],
        'Y':["101","101","010","010","010"],'Z':["111","001","010","100","111"],
        ' ':["000","000","000","000","000"],'!':["010","010","010","000","010"],
        '0':["010","101","101","101","010"],'1':["010","110","010","010","111"],
        '2':["110","001","010","100","111"],'3':["110","001","010","001","110"],
        '4':["101","101","111","001","001"],'5':["111","100","110","001","110"],
        '6':["011","100","110","101","010"],'7':["111","001","010","010","010"],
        '8':["010","101","010","101","010"],'9':["010","101","011","001","110"],
    }
    bitmap = font_3x5.get(ch, ["000","000","000","000","000"])
    for ry, row in enumerate(bitmap):
        for rx, px in enumerate(row):
            if px == '1':
                ox = cx + rx + 4; oy = cy + ry + 3
                if ox < atlas_w and oy < atlas_h:
                    idx = (oy * atlas_w + ox) * 4
                    font_atlas[idx:idx+4] = bytes([255,255,255,255])
write_png(os.path.join(FONT, 'bitmap_font.png'), atlas_w, atlas_h, bytes(font_atlas))

# font metrics json
with open(os.path.join(FONT, 'bitmap_font.json'), 'w') as f:
    import json
    json.dump({"image":"bitmap_font.png","glyph_w":glyph_w,"glyph_h":glyph_h,
               "cols":cols,"rows":rows,"first_char":32,"count":95}, f, indent=2)

print("Sample assets generated in", ROOT)
