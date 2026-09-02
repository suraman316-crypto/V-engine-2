#!/usr/bin/env python3
"""Generate additional large assets to push the APK over the 90MB target.
Adds more music tracks and a few extra high-res textures."""
import os, sys, struct, zlib, wave, math, random, json

ROOT = os.path.dirname(os.path.abspath(__file__))
AUDIO = os.path.join(ROOT, 'audio')
TEX = os.path.join(ROOT, 'textures')
SPRITE = os.path.join(ROOT, 'sprites')

def write_png(path, w, h, rgba, level=4):
    def chunk(typ, data):
        c = typ + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)
    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)
    raw = b''
    stride = w * 4
    for y in range(h):
        raw += b'\x00' + rgba[y*stride:(y+1)*stride]
    idat = zlib.compress(raw, level)
    with open(path, 'wb') as f:
        f.write(sig + chunk(b'IHDR', ihdr) + chunk(b'IDAT', idat) + chunk(b'IEND', b''))

def write_wav(path, samples, sr=44100, channels=2):
    with wave.open(path, 'wb') as w:
        w.setnchannels(channels); w.setsampwidth(2); w.setframerate(sr)
        frames = []
        for s in samples:
            v = max(-32768, min(32767, int(s*32767)))
            frames.append(struct.pack('<hh', v, v))
        w.writeframes(b''.join(frames))

def music_loop(path, dur, key_freq, sr=44100):
    n = int(sr * dur)
    out = [0.0] * n
    beat = sr * 0.5
    bass_notes = [key_freq, key_freq, key_freq*1.5, key_freq*0.75]
    for i in range(n):
        t = i / sr
        beat_idx = int(i / beat) % len(bass_notes)
        f = bass_notes[beat_idx]
        env = math.exp(-(i % beat) / (sr*0.3))
        out[i] += 0.2 * env * math.sin(2*math.pi*f*t)
    arp = [key_freq, key_freq*1.25, key_freq*1.5, key_freq*2.0]
    for i in range(n):
        t = i / sr
        step = int(t * 4) % len(arp)
        f = arp[step]
        env = math.exp(-(i % (sr//4)) / (sr*0.1))
        out[i] += 0.12 * env * math.sin(2*math.pi*f*t)
    for i in range(n):
        t = i / sr
        out[i] += 0.06 * (math.sin(2*math.pi*key_freq*t) + math.sin(2*math.pi*key_freq*1.5*t))
    fade = int(sr * 0.5)
    for i in range(min(fade, n)):
        out[i] *= i / fade
        out[n-1-i] *= i / fade
    write_wav(path, out, sr, 2)

def nebula_texture(path, w, h, seed, palette):
    rng = random.Random(seed)
    grid_w = max(2, w // 32); grid_h = max(2, h // 32)
    grid = [[rng.random() for _ in range(grid_w)] for _ in range(grid_h)]
    out = bytearray()
    for y in range(h):
        gy = y / h * (grid_h - 1)
        gy0 = int(gy); gy1 = min(gy0 + 1, grid_h - 1); fy = gy - gy0
        for x in range(w):
            gx = x / w * (grid_w - 1)
            gx0 = int(gx); gx1 = min(gx0 + 1, grid_w - 1); fx = gx - gx0
            a = grid[gy0][gx0]*(1-fx) + grid[gy0][gx1]*fx
            b = grid[gy1][gx0]*(1-fx) + grid[gy1][gx1]*fx
            v = a*(1-fy) + b*fy
            t = v
            c1, c2, c3 = palette
            if t < 0.5:
                tt = t * 2
                r = int(c1[0] + (c2[0]-c1[0])*tt)
                g = int(c1[1] + (c2[1]-c1[1])*tt)
                b_ = int(c1[2] + (c2[2]-c1[2])*tt)
            else:
                tt = (t-0.5)*2
                r = int(c2[0] + (c3[0]-c2[0])*tt)
                g = int(c2[1] + (c3[1]-c2[1])*tt)
                b_ = int(c2[2] + (c3[2]-c2[2])*tt)
            a_ = int(255 * (0.3 + 0.7*t))
            out += bytes([r, g, b_, a_])
    write_png(path, w, h, bytes(out), level=4)

def main():
    # extra music tracks (level 4-6 + cutscenes), 35s each
    extra_tracks = [
        ('level4_theme', 35.0, 392.0),    # G4
        ('level5_theme', 35.0, 440.0),    # A4
        ('level6_theme', 35.0, 493.88),   # B4
        ('cutscene_theme', 40.0, 261.63),# C4
        ('credits_theme', 45.0, 329.63), # E4
        ('tutorial_theme', 30.0, 246.94),# B3
    ]
    for name, dur, freq in extra_tracks:
        p = os.path.join(AUDIO, f'{name}.wav')
        if not os.path.exists(p):
            music_loop(p, dur, freq)
            print(f"  {name}.wav ({os.path.getsize(p)//1024}KB)")

    # extra ambient beds (90s each)
    for name, dur in [('deep_space', 90.0), ('cave_ambient', 90.0), ('city_ambient', 90.0)]:
        p = os.path.join(AUDIO, f'{name}.wav')
        if not os.path.exists(p):
            sr = 44100; n = int(sr*dur)
            rng = random.Random(hash(name)&0xffff)
            out = [0.0]*n
            for i in range(n):
                t = i/sr
                out[i] = 0.05*rng.gauss(0,1)*math.exp(-(i% (sr//4))/(sr*0.2))
                out[i] += 0.03*math.sin(2*math.pi*55*t)
            write_wav(p, out, sr, 2)
            print(f"  {name}.wav ({os.path.getsize(p)//1024}KB)")

    # extra high-res nebulae (2048)
    extra_neb = [
        ('nebula_cyan', 6, [(5,40,60),(20,120,140),(80,200,220)]),
        ('nebula_gold', 7, [(40,30,5),(120,90,20),(220,180,40)]),
        ('nebula_magenta', 8, [(40,5,40),(120,30,90),(220,80,200)]),
        ('nebula_teal', 9, [(5,40,30),(20,100,90),(60,180,160)]),
    ]
    for name, seed, pal in extra_neb:
        p = os.path.join(TEX, f'{name}_2048.png')
        if not os.path.exists(p):
            nebula_texture(p, 2048, 2048, seed, pal)
            print(f"  {name}_2048.png ({os.path.getsize(p)//1024}KB)")

    # large character portraits (1024x1024)
    portraits = [
        ('pilot_a', 20, (180,140,90)), ('pilot_b', 21, (90,120,180)),
        ('enemy_a', 22, (160,60,60)), ('enemy_b', 23, (90,40,140)),
    ]
    for name, seed, col in portraits:
        p = os.path.join(SPRITE, f'{name}.png')
        if not os.path.exists(p):
            nebula_texture(p, 1024, 1024, seed, [col, (col[0]//2,col[1]//2,col[2]//2), col])

    total = 0
    for r,_,fs in os.walk(ROOT):
        for fn in fs:
            total += os.path.getsize(os.path.join(r,fn))
    print(f"Total assets size: {total//1024//1024}MB")

if __name__ == '__main__':
    main()
