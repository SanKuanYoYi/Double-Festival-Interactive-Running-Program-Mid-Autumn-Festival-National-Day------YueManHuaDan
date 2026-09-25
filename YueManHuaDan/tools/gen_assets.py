# -*- coding: utf-8 -*-
"""
YueManHuaDan 素材生成脚本（纯 Python 标准库，无需 Pillow / numpy）

生成 4 张带透明通道的 PNG，供 resources.qrc 引用：
    images/moon.png         月亮本体 + 外层暖光
    images/mooncake.png     广式月饼俯视图（游戏里再叠加文字）
    images/spark.png        粒子光点（代码粒子、灯笼光斑）
    images/glow.png         灯笼 / 路灯光晕

用法：
    python tools/gen_assets.py
"""

import math
import os
import struct
import zlib


# --------------------------------------------------------------------------- #
# 最小 PNG 写出器
# --------------------------------------------------------------------------- #
def write_png(path, width, height, rows):
    """rows: 列表，每行是长度 width*4 的 RGBA 字节序列"""
    raw = bytearray()
    stride = width * 4
    for row in rows:
        raw.append(0)              # filter type 0
        raw.extend(row[:stride])

    def chunk(tag, data):
        body = tag + data
        return (struct.pack(">I", len(data)) + body
                + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF))

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    blob = (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", header)
            + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
            + chunk(b"IEND", b""))
    with open(path, "wb") as fp:
        fp.write(blob)
    print("written: %s (%dx%d, %.1f KB)" % (path, width, height, len(blob) / 1024.0))


def clamp01(v):
    return 0.0 if v < 0.0 else (1.0 if v > 1.0 else v)


def smooth_step(edge0, edge1, x):
    t = clamp01((x - edge0) / (edge1 - edge0 + 1e-9))
    return t * t * (3.0 - 2.0 * t)


def mix(a, b, t):
    return a + (b - a) * t


# --------------------------------------------------------------------------- #
# 月亮
# --------------------------------------------------------------------------- #
def make_moon(size=512):
    rows = []
    cx = cy = size / 2.0
    radius = size * 0.40
    craters = ((0.28, -0.24, 0.155), (-0.36, 0.16, 0.125), (0.06, 0.38, 0.10),
               (-0.12, -0.42, 0.085), (0.46, 0.28, 0.095), (-0.48, -0.10, 0.065))

    for y in range(size):
        row = bytearray()
        ny = (y - cy) / radius
        for x in range(size):
            nx = (x - cx) / radius
            dist = math.hypot(nx, ny)
            if dist <= 1.0:
                lum = 0.88 + 0.12 * (-nx * 0.55 - ny * 0.55)
                for crater_x, crater_y, crater_r in craters:
                    d = math.hypot(nx - crater_x, ny - crater_y)
                    if d < crater_r:
                        lum -= 0.11 * (1.0 - d / crater_r)
                    elif d < crater_r * 1.22:
                        lum += 0.045
                lum += 0.013 * math.sin(nx * 13.0) * math.sin(ny * 11.0)
                lum = clamp01(lum)
                r = 255 * min(1.0, lum * 1.03)
                g = 249 * lum
                b = 227 * lum * 0.98
                a = 255 * (1.0 - smooth_step(0.975, 1.0, dist))
            else:
                halo = math.exp(-((dist - 1.0) * 5.0) ** 2) * 0.62
                wide = math.exp(-((dist - 1.0) * 1.55) ** 2) * 0.20
                intensity = clamp01(halo + wide)
                r, g, b = 255, mix(226, 200, clamp01(dist - 1.0)), mix(196, 150, clamp01(dist - 1.0))
                a = 255 * intensity
            row += bytes((int(r), int(g), int(b), int(a)))
        rows.append(row)
    return rows


# --------------------------------------------------------------------------- #
# 月饼
# --------------------------------------------------------------------------- #
def make_mooncake(size=256):
    rows = []
    cx = cy = size / 2.0
    base = size * 0.455

    for y in range(size):
        row = bytearray()
        for x in range(size):
            dx = x - cx
            dy = y - cy
            dist = math.hypot(dx, dy)
            theta = math.atan2(dy, dx)
            nx, ny = dx / base, dy / base

            # 模具花瓣边（广式月饼的锯齿外沿）
            edge = base * (1.0 + 0.048 * math.cos(16 * theta) + 0.016 * math.cos(32 * theta))
            if dist > edge:
                row += bytes((0, 0, 0, 0))
                continue

            # 主体：左上受光的金棕色
            shade = clamp01(0.5 - nx * 0.32 - ny * 0.34)
            r = mix(168, 235, shade)
            g = mix(108, 176, shade)
            b = mix(42, 84, shade)

            # 外圈略深的饼边
            rim = smooth_step(0.80, 0.88, dist / base)
            r, g, b = mix(r, 150, rim * 0.55), mix(g, 96, rim * 0.55), mix(b, 36, rim * 0.55)

            # 内圈凸起的压花
            inner = dist / base
            if inner < 0.70:
                rr, gg, bb = mix(r, 226, 0.35), mix(g, 168, 0.35), mix(b, 72, 0.35)
                # 花瓣压纹
                petal = abs(math.cos(4.0 * theta + math.pi / 4.0))
                if 0.30 < inner < 0.62 and petal > 0.62:
                    rr, gg, bb = 144.0, 92.0, 34.0
                # 中心方胜纹
                ax, ay = abs(nx), abs(ny)
                if max(ax, ay) < 0.155 or (ax + ay) < 0.175:
                    rr, gg, bb = 236.0, 186.0, 96.0
                r, g, b = rr, gg, bb

            # 顶部高光
            spec = clamp01(0.62 - math.hypot(nx + 0.34, ny + 0.38) * 1.9)
            r, g, b = mix(r, 255, spec * 0.5), mix(g, 245, spec * 0.5), mix(b, 210, spec * 0.5)

            alpha = 255 * (1.0 - smooth_step(0.955, 1.0, dist / edge))
            row += bytes((int(r), int(g), int(b), int(alpha)))
        rows.append(row)
    return rows


# --------------------------------------------------------------------------- #
# 光点 / 光晕
# --------------------------------------------------------------------------- #
def make_spark(size=64):
    rows = []
    cx = cy = size / 2.0
    for y in range(size):
        row = bytearray()
        for x in range(size):
            dist = math.hypot(x - cx, y - cy) / (size / 2.0)
            core = clamp01(1.0 - dist) ** 2.4
            aura = math.exp(-(dist * 2.3) ** 2) * 0.55
            i = clamp01(core + aura)
            row += bytes((255, int(mix(232, 255, core)), int(mix(168, 236, core)), int(255 * i)))
        rows.append(row)
    return rows


def make_glow(size=256):
    rows = []
    cx = cy = size / 2.0
    for y in range(size):
        row = bytearray()
        for x in range(size):
            dist = math.hypot(x - cx, y - cy) / (size / 2.0)
            i = clamp01(math.exp(-(dist * 1.9) ** 2))
            warm = clamp01(dist)
            row += bytes((255, int(mix(206, 255, 1 - warm)), int(mix(120, 220, 1 - warm)), int(255 * i * 0.95)))
        rows.append(row)
    return rows


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    out_dir = os.path.join(os.path.dirname(here), "images")
    os.makedirs(out_dir, exist_ok=True)

    jobs = (("moon.png", make_moon(512)),
            ("mooncake.png", make_mooncake(256)),
            ("spark.png", make_spark(64)),
            ("glow.png", make_glow(256)))
    for name, rows in jobs:
        write_png(os.path.join(out_dir, name), len(rows[0]) // 4, len(rows), rows)


if __name__ == "__main__":
    main()
