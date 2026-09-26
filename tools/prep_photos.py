# -*- coding: utf-8 -*-
"""
prep_photos.py —— 实拍摄影素材预处理脚本
========================================

用途
----
把 photos/ 里的原始照片，处理成程序所需的 16:9 / 1:1 画幅，
输出到 images/ 供 resources.qrc 打包。

处理的四件事
------------
1. 转正：school_night 原文件是「横构图照片被旋转 90° 存成了竖图」，
   逆时针转 90° 还原成 2048x1152，再降采样到 1920x1080。
2. 去水印：anyang_projection 底部有社交平台水印，整条裁掉；
   yinxu_museum 画面正中有一道贯穿的「×」形半透明水印（两条 ±45° 细线），
   用霍夫直线检测定位后做 inpaint 修补，保证输出画面「无水印等瑕疵」。
3. 重构图：竖构图原图直接塞进 16:9 的画面会被严重裁切、塞进 1:1 圆幕会被拉伸。
   这里以主体为中心，两侧用「镜像 + 高斯模糊 + 压暗」做氛围延展，主体保持清晰、比例不失真。
4. 补方幕：殷墟博物馆原图是 3:2 横构图，圆幕是 1:1，主体置上、
   下方用垂直镜像做「水面倒影」补满，得到 512x512 的「甲骨·国」投影画面。

运行
----
    python tools/prep_photos.py

依赖：Pillow（pip install Pillow）；
      去水印与降噪需要 opencv-python（pip install opencv-python），
      未安装时这两步会自动跳过，其余步骤（构图、影调）仍可用 Pillow 完成。
若 photos/ 中缺少某张原图，对应的输出会跳过，程序仍会回退到内置矢量绘制。
"""
import os
import sys

try:
    from PIL import Image, ImageFilter, ImageEnhance, ImageChops, ImageDraw
except ImportError:
    sys.exit("需要 Pillow：pip install Pillow")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "photos")
DST = os.path.join(ROOT, "images")
os.makedirs(DST, exist_ok=True)


# ---------------------------------------------------------------- 工具函数
def mirror_strip(panel, width):
    """把 panel 左右交替镜像拼接，取居中 width 宽的一段（用于画面延展）。"""
    if panel.width >= width:
        left = (panel.width - width) // 2
        return panel.crop((left, 0, left + width, panel.height))

    seq, total, flip = [panel], panel.width, True
    while total < width + panel.width:
        seq.append(panel.transpose(Image.FLIP_LEFT_RIGHT) if flip else panel)
        total += panel.width
        flip = not flip

    strip = Image.new("RGB", (total, panel.height))
    x = 0
    for s in seq:
        strip.paste(s, (x, 0))
        x += s.width
    left = (total - width) // 2
    return strip.crop((left, 0, left + width, panel.height))


def center_focus_mask(w, h, inner):
    """中心清晰、向外线性过渡到模糊的遮罩。"""
    mask = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(mask)
    cx = w / 2.0
    for x in range(w):
        t = abs(x - cx) / cx
        v = 255 if t <= inner else int(255 * max(0.0, 1.0 - (t - inner) / (1.0 - inner)))
        d.line([(x, 0), (x, h)], fill=v)
    return mask.filter(ImageFilter.GaussianBlur(38))


def edge_vignette(img, strength=0.85, power=2.2):
    """左右 / 上下压暗的柔光暗角。"""
    w, h = img.size
    mask = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(mask)
    cx = w / 2.0
    for x in range(w):
        d.line([(x, 0), (x, h)], fill=int(255 * strength * ((abs(x - cx) / cx) ** power)))
    mask2 = Image.new("L", (w, h), 0)
    d2 = ImageDraw.Draw(mask2)
    cy = h / 2.0
    for y in range(h):
        d2.line([(0, y), (w, y)], fill=int(255 * strength * 0.75 * ((abs(y - cy) / cy) ** power)))
    dark = ImageChops.lighter(mask, mask2).filter(ImageFilter.GaussianBlur(60))
    return Image.composite(Image.new("RGB", (w, h), (5, 7, 16)), img, dark)


def extend_horizontally(panel, target_w, target_h, blur=52, dim=0.42, vignette=0.8):
    """竖构图主体 -> 目标画幅：两侧镜像+模糊+压暗，中间保持清晰。"""
    if panel.height != target_h:
        nw = max(1, int(round(panel.width * target_h / panel.height)))
        panel = panel.resize((nw, target_h), Image.LANCZOS)

    strip = mirror_strip(panel, target_w)

    bg = strip.filter(ImageFilter.GaussianBlur(blur))
    bg = ImageEnhance.Brightness(bg).enhance(dim)
    bg = ImageEnhance.Color(bg).enhance(0.8)

    fg = strip.filter(ImageFilter.GaussianBlur(1.2))

    # 清晰范围恰好覆盖主体宽度，从主体边缘开始过渡，不留竖直硬边
    inner = max(0.25, min(0.86, (panel.width / float(target_w)) * 0.94))
    canvas = Image.composite(fg, bg, center_focus_mask(target_w, target_h, inner))
    return edge_vignette(canvas, strength=vignette)


def extend_vertically_water(panel, target_w, target_h):
    """横构图主体 -> 1:1 方幕：主体居中，上下用垂直镜像延展补满，下半压暗似水面。

    殷墟博物馆夜景原图是 3:2 横构图，圆幕却是 1:1。把建筑居中放稳，
    上下缺的部分用「过主体边缘做镜像」的延展补齐，再把下半部压暗成夜色水面，
    避免直接留黑边或把画面拉扁。这样建筑如悬在夜色里，正合月光投影的气质。
    """
    if panel.width != target_w:
        nh = max(1, int(round(panel.height * target_w / float(panel.width))))
        panel = panel.resize((target_w, nh), Image.LANCZOS)

    pw, ph = panel.size
    canvas = Image.new("RGB", (target_w, target_h), (3, 4, 10))
    if ph >= target_h:
        top = (ph - target_h) // 2
        canvas.paste(panel.crop((0, top, pw, top + target_h)), (0, 0))
        return canvas

    top = (target_h - ph) // 2
    canvas.paste(panel, (0, top))

    flipped = panel.transpose(Image.FLIP_TOP_BOTTOM)
    if top > 0:                                      # 上方镜像延展（贴住主体上边缘）
        canvas.paste(flipped.crop((0, ph - top, pw, ph)), (0, 0))
    below = target_h - top - ph
    if below > 0:                                    # 下方镜像延展（贴住主体下边缘）
        canvas.paste(flipped.crop((0, 0, pw, below)), (0, top + ph))

    # 下半部（水线以下）渐次压暗成夜色，越往底越黑
    waterline = top + ph
    dark = Image.new("RGB", (target_w, 1), (3, 4, 10))
    for y in range(waterline, target_h):
        t = (y - waterline) / float(max(1, target_h - waterline))
        canvas.paste(Image.blend(canvas.crop((0, y, target_w, y + 1)), dark,
                                 min(0.92, 0.25 + t * 0.7)), (0, y))

    # 顶部延展（镜像的天空）略压暗，别抢主体
    for y in range(0, top):
        t = 1.0 - y / float(max(1, top))
        canvas.paste(Image.blend(canvas.crop((0, y, target_w, y + 1)), dark,
                                 t * 0.55), (0, y))

    return canvas


def load(name):
    path = os.path.join(SRC, name)
    if not os.path.exists(path):
        print("  [跳过] 找不到 photos/%s" % name)
        return None
    return Image.open(path).convert("RGB")


# ---------------------------------------------------------------- 1) 校园夜景
def build_school_night():
    im = load("school_night_raw.jpg")
    if im is None:
        return
    print("school_night 原图:", im.size)
    land = im.rotate(90, expand=True)          # 横构图被存成竖图，转正
    print("  转正后:", land.size)
    land = land.resize((1920, 1080), Image.LANCZOS)
    land = ImageEnhance.Contrast(land).enhance(1.06)
    land = ImageEnhance.Color(land).enhance(1.05)
    land = ImageEnhance.Brightness(land).enhance(1.04)
    land.save(os.path.join(DST, "school_night.jpg"), "JPEG", quality=90, optimize=True)
    print("  -> images/school_night.jpg", land.size)


# ---------------------------------------------------------------- 2) 安阳文峰塔
def build_anyang():
    im = load("anyang_tower_raw.jpg")
    if im is None:
        return
    w, h = im.size
    print("anyang 原图:", im.size)
    tree = im.crop((0, 0, w, h - 62))          # 裁掉底部平台水印
    print("  去水印后:", tree.size)
    tree = ImageEnhance.Brightness(tree).enhance(1.10)
    tree = ImageEnhance.Contrast(tree).enhance(1.08)
    proj = extend_horizontally(tree, 1024, 1024, blur=56, dim=0.34, vignette=0.72)
    proj.save(os.path.join(DST, "anyang_projection.jpg"), "JPEG", quality=92, optimize=True)
    print("  -> images/anyang_projection.jpg", proj.size)


# ---------------------------------------------------------------- 3) 校训楼夜色
def build_ending():
    im = load("campus_motto_raw.jpg")
    if im is None:
        return
    W, H = im.size
    print("ending_card 原图:", im.size)
    # 只取「屋檐 + 国旗 + 上部立面」，止于校训墙之前：
    # 一是镜像延展时中文字会反显，二是校训文字交给结尾页的矢量校训石承载。
    body = im.crop((0, 60, W, 1030))
    print("  取主体:", body.size)
    body = ImageEnhance.Brightness(body).enhance(1.06)
    body = ImageEnhance.Contrast(body).enhance(1.05)
    ending = extend_horizontally(body, 1920, 1080, blur=64, dim=0.34, vignette=0.86)
    ending.save(os.path.join(DST, "ending_card.jpg"), "JPEG", quality=90, optimize=True)
    print("  -> images/ending_card.jpg", ending.size)

    # 顺带把原图书法「楼倚暮霞 德韵扬辉」抠成透明 PNG，缀在结尾页右下角
    cal = im.crop((70, 1400, 1030, 1800)).convert("L")
    alpha = cal.point(lambda v: 0 if v < 96 else min(255, int((v - 96) * 255 / 92)))
    alpha = alpha.filter(ImageFilter.GaussianBlur(0.6)).point(lambda v: 0 if v < 40 else v)
    ink = Image.new("RGB", cal.size, (255, 248, 232))
    out = Image.merge("RGBA", (ink.split()[0], ink.split()[1], ink.split()[2], alpha))
    bbox = alpha.point(lambda v: 255 if v > 24 else 0).getbbox()
    if bbox:
        out = out.crop(bbox)
    out.thumbnail((900, 400), Image.LANCZOS)
    out.save(os.path.join(DST, "ending_motto.png"), "PNG", optimize=True)
    print("  -> images/ending_motto.png", out.size)


# ---------------------------------------------------------------- 4) 殷墟博物馆夜景
def build_yinxu():
    """殷墟博物馆（甲骨）夜景 -> 「甲骨·国」投影圆幕（512x512）。

    原图正中有一道贯穿画面的「×」形半透明水印（两条 ±45° 细线），
    先用霍夫直线把它们找出来、inpaint 修补掉，再降噪提亮、补成方幕。
    """
    im = load("yinxu_museum_raw.webp")
    if im is None:
        return
    print("yinxu 原图:", im.size)

    try:
        import cv2
        import numpy as np
    except ImportError:
        print("  [警告] 未装 opencv-python，跳过去水印与降噪（成品会残留水印）")
        cv2 = None

    if cv2 is not None:
        bgr = cv2.cvtColor(np.array(im), cv2.COLOR_RGB2BGR)
        # 1) 检出水印斜线：Canny 边缘 + 概率霍夫，只留 ±45°±7° 的线段
        gray = cv2.cvtColor(bgr, cv2.COLOR_BGR2GRAY)
        edges = cv2.Canny(gray, 10, 40)
        lines = cv2.HoughLinesP(edges, 1, np.pi / 180.0, threshold=32,
                                minLineLength=55, maxLineGap=22)
        mask = np.zeros_like(gray)
        hits = 0
        if lines is not None:
            for x1, y1, x2, y2 in lines.reshape(-1, 4):
                ang = abs(np.degrees(np.arctan2(y2 - y1, x2 - x1)))
                if 38.0 <= ang <= 52.0:
                    cv2.line(mask, (x1, y1), (x2, y2), 255, 3)
                    hits += 1
        print("  检出斜向水印线段:", hits)
        if hits:
            mask = cv2.dilate(mask, np.ones((3, 3), np.uint8), iterations=1)
            bgr = cv2.inpaint(bgr, mask, 4, cv2.INPAINT_TELEA)
        # 2) 夜景噪点较重，先做非局部均值降噪
        bgr = cv2.fastNlMeansDenoisingColored(bgr, None, 6, 6, 7, 21)
        # 3) LAB 空间对亮度通道做 CLAHE，提亮暗部又不过曝
        lab = cv2.cvtColor(bgr, cv2.COLOR_BGR2LAB)
        l, a, b = cv2.split(lab)
        l = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8, 8)).apply(l)
        bgr = cv2.cvtColor(cv2.merge((l, a, b)), cv2.COLOR_LAB2BGR)
        im = Image.fromarray(cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB))

    # 4) 影调微调：夜景整体偏暗，略提亮、略加对比与饱和
    im = ImageEnhance.Brightness(im).enhance(1.06)
    im = ImageEnhance.Contrast(im).enhance(1.08)
    im = ImageEnhance.Color(im).enhance(1.15)

    # 5) 补成 512x512 方幕（主体在上，水面倒影在下）
    proj = extend_vertically_water(im, 512, 512)
    proj.save(os.path.join(DST, "yinxu_projection.jpg"), "JPEG", quality=92, optimize=True)
    print("  -> images/yinxu_projection.jpg", proj.size)


if __name__ == "__main__":
    print("素材来源目录:", SRC)
    print("成品输出目录:", DST, "\n")
    build_school_night()
    build_anyang()
    build_ending()
    build_yinxu()
    print("\n完成。")
