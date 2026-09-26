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
    """满月本体。

    注意两处关键细节（都会直接变成画面瑕疵）：
    1. 不画那种延伸到画布四角的宽光晕——它会变成一个"方形亮块"叠在实拍照片上；
       这里只保留贴着月面的一圈薄辉，1.35 倍半径以外完全透明。光晕由代码里的
       QRadialGradient 负责，两者叠加才有层次。
    2. 完全透明的像素仍然写入暖白色而不是 (0,0,0,0)——否则 Qt 做双线性缩放时
       会把黑边拖进月面，月亮周围就会出现一圈脏兮兮的黑环。
    """
    rows = []
    cx = cy = size / 2.0
    radius = size * 0.40

    # 环形山：(中心x, 中心y, 外沿半径, 碗口半径)，坐标已按月面半径归一化
    craters = (
        (0.30, -0.26, 0.148, 0.106), (-0.37, 0.14, 0.120, 0.090),
        (0.02, 0.40, 0.102, 0.076), (-0.14, -0.44, 0.084, 0.063),
        (0.47, 0.27, 0.094, 0.071), (-0.50, -0.12, 0.060, 0.046),
        (0.60, -0.08, 0.051, 0.039), (-0.28, 0.52, 0.067, 0.051),
        (0.18, 0.16, 0.132, 0.097), (-0.06, -0.14, 0.077, 0.056),
        (0.40, 0.52, 0.046, 0.035), (-0.58, 0.36, 0.053, 0.041),
        (0.10, -0.62, 0.049, 0.037),
    )
    # 月海：成片的低对比暗区，让月面不至于是一块死白
    maria = ((0.24, 0.34, 0.40), (-0.34, -0.28, 0.30), (0.52, -0.42, 0.22))

    for y in range(size):
        row = bytearray()
        for x in range(size):
            nx = (x - cx) / radius
            ny = (y - cy) / radius
            dist = math.hypot(nx, ny)
            if dist < 1.0:
                # 球面法线 → 兰伯特光照，光源来自左上
                nz = math.sqrt(max(0.0, 1.0 - dist * dist))
                lam = clamp01(nx * -0.42 + ny * -0.52 + nz * 0.74)
                # 明暗对比刻意压得很浅：满月的天光本来就很平，而且一旦把边缘压暗，
                # 外侧那圈暖辉就会把暗边衬成一根"描边"，非常假。
                # 末段再稍微回亮一点，模拟满月的冲日效应。
                lum = 0.845 + 0.175 * lam
                lum += 0.062 * smooth_step(0.90, 0.995, dist)

                for mx, my, mr in maria:
                    dd = math.hypot(nx - mx, ny - my)
                    if dd < mr:
                        lum -= 0.048 * (1.0 - dd / mr) ** 0.7
                for crx, cry, crr, crd in craters:
                    dd = math.hypot(nx - crx, ny - cry)
                    # 加一点点角度扰动，免得环形山全是一个个死圆的"泡泡"
                    dd *= 1.0 + 0.045 * math.sin(3.0 * math.atan2(ny - cry, nx - crx) + crx * 9.0)
                    if dd < crd:
                        lum -= 0.085 * (1.0 - dd / crd) ** 0.75
                    elif dd < crr:
                        # 外沿山脊只给极小的一点提亮，给多了就变成肥皂泡
                        lum += 0.010 * math.sin(math.pi * (dd - crd) / max(1e-6, crr - crd))

                # 细腻的月面颗粒，避免大片纯色
                lum += 0.007 * math.sin(nx * 17.0) * math.sin(ny * 15.0)
                lum += 0.004 * math.sin(nx * 41.0 + 1.7) * math.sin(ny * 37.0)
                lum = clamp01(lum)

                r = min(255.0, 255.0 * lum * 1.00)
                g = min(255.0, 248.0 * lum * 1.02)
                b = min(255.0, 228.0 * lum * 1.04)
                a = 255.0 * (1.0 - smooth_step(0.968, 1.0, dist))
            else:
                # 月面之外一律全透明，光晕完全交给代码里的 QRadialGradient。
                # 如果 PNG 自己再带一圈亮辉，就会把月面边缘那点轻微的暗化
                # 夹在"内亮—外亮"中间，看起来像给月亮描了一根边。
                r, g, b = 255.0, 238.0, 200.0
                a = 0.0
            row += bytes((int(r), int(g), int(b), int(a)))
        rows.append(row)
    return rows


# --------------------------------------------------------------------------- #
# 月饼
# --------------------------------------------------------------------------- #
def make_mooncake(size=256):
    """广式月饼俯视图。

    关键在于外沿：千万不要用高次余弦（比如 cos(16θ)）去做"锯齿花边"，
    那样画出来是一枚齿轮，而不是月饼。这里用 10 瓣、振幅 3% 的柔和波浪，
    再叠一道模压凹槽、一圈连纹、八瓣压花和中央圆台，才是一块饼。
    """
    rows = []
    cx = cy = size / 2.0
    radius = size * 0.44

    for y in range(size):
        row = bytearray()
        for x in range(size):
            dx = x - cx
            dy = y - cy
            dist = math.hypot(dx, dy)
            theta = math.atan2(dy, dx)

            # 饼模外沿：10 瓣柔和花边（振幅压到 3% 以下，否则就成了齿轮的齿）
            edge = radius * (1.0 + 0.024 * math.cos(10.0 * theta)
                                   + 0.007 * math.cos(20.0 * theta))
            if dist >= edge:
                # 外侧透明像素保留饼边色，避免缩放时拖出黑边
                row += bytes((140, 84, 28, 0))
                continue

            u = dist / edge
            nx, ny = dx / edge, dy / edge
            nz = math.sqrt(max(0.0, 1.0 - min(1.0, u * u)))
            lam = clamp01(nx * -0.38 + ny * -0.46 + nz * 0.80)
            shade = clamp01(0.30 + 0.78 * lam)

            # 饼皮：暗棕 → 金棕
            r = mix(150.0, 246.0, shade)
            g = mix(92.0, 186.0, shade)
            b = mix(34.0, 96.0, shade)

            # 外圈饼边略深，带一点烘烤色
            rim = smooth_step(0.855, 0.905, u)
            r = mix(r, 132.0, rim * 0.60)
            g = mix(g, 78.0, rim * 0.60)
            b = mix(b, 26.0, rim * 0.60)

            # 饼边内侧一道模压凹槽
            groove = math.exp(-((u - 0.815) * 34.0) ** 2)
            r = mix(r, 118.0, groove * 0.70)
            g = mix(g, 66.0, groove * 0.70)
            b = mix(b, 22.0, groove * 0.70)

            # 凹槽外沿的连纹（12 段短弧）
            if 0.845 < u < 0.900:
                seg = abs(math.sin(6.0 * theta))
                r = mix(r, 176.0, seg * 0.32)
                g = mix(g, 116.0, seg * 0.32)
                b = mix(b, 46.0, seg * 0.32)

            # 中央凸起的饼面
            if u < 0.700:
                rr = mix(206.0, 250.0, shade)
                gg = mix(146.0, 198.0, shade)
                bb = mix(58.0, 106.0, shade)
                # 一圈压花圆点（月饼模具最常见的纹样）；
                # 这里刻意不用从圆心辐射的楔形，那会立刻变成齿轮的齿
                for k in range(10):
                    ang = k * math.tau / 10.0 + 0.31
                    ddx = nx - 0.455 * math.sin(ang)
                    ddy = ny - 0.455 * -math.cos(ang)
                    if math.hypot(ddx, ddy) < 0.088:
                        rr, gg, bb = rr * 0.76 + 96.0 * 0.24, gg * 0.76 + 58.0 * 0.24, bb * 0.76 + 20.0 * 0.24
                # 中央圆台（月饼中央那枚印记）
                if u < 0.185:
                    rr = mix(238.0, 255.0, shade)
                    gg = mix(188.0, 228.0, shade)
                    bb = mix(96.0, 152.0, shade)
                elif u < 0.215:
                    rr, gg, bb = rr * 0.70, gg * 0.70, bb * 0.70
                # 饼面与饼边之间的第二道模压凹槽
                g2 = math.exp(-((u - 0.655) * 40.0) ** 2)
                rr, gg, bb = mix(rr, 130.0, g2 * 0.55), mix(gg, 76.0, g2 * 0.55), mix(bb, 28.0, g2 * 0.55)
                r, g, b = rr, gg, bb

            # 左上高光
            spec = clamp01(0.60 - math.hypot(nx + 0.36, ny + 0.40) * 1.75)
            r = mix(r, 255.0, spec * 0.45)
            g = mix(g, 246.0, spec * 0.45)
            b = mix(b, 206.0, spec * 0.45)

            alpha = 255.0 * (1.0 - smooth_step(0.955, 1.0, u))
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
