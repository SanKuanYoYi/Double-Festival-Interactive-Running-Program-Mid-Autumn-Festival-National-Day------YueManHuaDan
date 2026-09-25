#ifndef THEME_H
#define THEME_H

/*
 * theme.h
 * 《月满华诞，码上团圆》全局配色 / 字体 / 通用绘制小工具
 *
 * 说明：中文串统一用 QStringLiteral，Qt5 / Qt6 下均不受执行字符集影响，
 *      避免 MSVC 编译时出现乱码。源文件建议在其它 include 之前先包含本文件。
 */

#if defined(Q_CC_MSVC)
#  pragma execution_character_set("utf-8")
#endif

#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QString>

#include <cmath>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace Theme {

// 逻辑分辨率：所有画面按 1280x720 布局，再由各自的视图等比缩放（letterbox）
constexpr int kSceneW = 1280;
constexpr int kSceneH = 720;

/* ---------------- 配色：红 · 金 · 白 ---------------- */
inline QColor nightTop()     { return QColor(8, 12, 27); }
inline QColor nightBottom()  { return QColor(26, 36, 64); }
inline QColor building()     { return QColor(13, 18, 34); }
inline QColor buildingEdge() { return QColor(42, 54, 86); }
inline QColor glassDark()    { return QColor(20, 27, 46); }
inline QColor chinaRed()     { return QColor(206, 26, 36); }
inline QColor deepRed()      { return QColor(142, 14, 26); }
inline QColor gold()         { return QColor(238, 186, 88); }
inline QColor lightGold()    { return QColor(255, 229, 168); }
inline QColor cream()        { return QColor(255, 249, 233); }
inline QColor warmWhite()    { return QColor(252, 246, 236); }
inline QColor windowLit()    { return QColor(255, 205, 122); }
inline QColor codeGreen()    { return QColor(112, 232, 178); }
inline QColor ink()          { return QColor(12, 15, 26); }

/* ---------------- 字体 ---------------- */
inline QFont font(int pixelSize, bool bold = false)
{
    QFont f;
    f.setPixelSize(pixelSize);
    f.setBold(bold);
    f.setStyleHint(QFont::SansSerif);
    // 找一个真实存在的中文字体；一个都没有就保持系统默认字体，
    // 让 Qt 自己的字形回退链去处理（比硬塞一个不存在的字体名安全）。
    static const QStringList installedFamilies = [] {
        QFontDatabase db;
        return db.families();
    }();

    const char *candidates[] = { "Microsoft YaHei", "微软雅黑",
                                 "Source Han Sans SC", "Noto Sans CJK SC",
                                 "WenQuanYi Micro Hei", "SimHei", "PingFang SC", "Heiti SC" };
    for (const char *name : candidates) {
        const QString family = QString::fromUtf8(name);
        if (installedFamilies.contains(family, Qt::CaseInsensitive)) {
            f.setFamily(family);
            break;
        }
    }
    return f;
}

/* ---------------- 数值小工具 ---------------- */
inline qreal clamp01(qreal v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }
inline qreal lerp(qreal a, qreal b, qreal t) { return a + (b - a) * t; }
inline qreal easeOutCubic(qreal t) { return 1.0 - std::pow(1.0 - clamp01(t), 3.0); }
inline qreal easeOutBack(qreal t)
{
    t = clamp01(t);
    const qreal c1 = 1.70158, c3 = c1 + 1.0;
    return 1.0 + c3 * std::pow(t - 1.0, 3.0) + c1 * std::pow(t - 1.0, 2.0);
}
inline qreal easeInOutQuad(qreal t)
{
    t = clamp01(t);
    return t < 0.5 ? 2.0 * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 2.0) / 2.0;
}

inline QPointF lerpPoint(const QPointF &a, const QPointF &b, qreal t)
{
    return QPointF(lerp(a.x(), b.x(), t), lerp(a.y(), b.y(), t));
}

inline QColor mixColor(const QColor &a, const QColor &b, qreal t)
{
    t = clamp01(t);
    return QColor(int(lerp(a.red(), b.red(), t)),
                  int(lerp(a.green(), b.green(), t)),
                  int(lerp(a.blue(), b.blue(), t)),
                  int(lerp(a.alpha(), b.alpha(), t)));
}

inline QColor withAlpha(const QColor &c, int alpha)
{
    QColor out = c;
    out.setAlpha(alpha);
    return out;
}

inline QRectF centeredRect(const QPointF &center, qreal w, qreal h)
{
    return QRectF(center.x() - w / 2.0, center.y() - h / 2.0, w, h);
}

/* ---------------- 绘制小工具 ---------------- */

// 带光晕的文字（以 center 为几何中心对齐）
inline void drawGlowText(QPainter *p, const QPointF &center, const QString &text,
                         const QColor &fill, const QColor &glow, qreal glowWidth = 6.0)
{
    p->save();
    const QFontMetrics fm(p->font());
    const qreal x = center.x() - fm.horizontalAdvance(text) / 2.0;
    const qreal y = center.y() + (fm.ascent() - fm.descent()) / 2.0;
    QPainterPath path;
    path.addText(x, y, p->font(), text);
    if (glow.isValid() && glowWidth > 0.0) {
        QPen pen(withAlpha(glow, 95), glowWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p->setPen(pen);
        p->drawPath(path);
    }
    p->setPen(Qt::NoPen);
    p->setBrush(fill);
    p->drawPath(path);
    p->restore();
}

// 圆角按键
inline void drawButton(QPainter *p, const QRectF &rect, const QString &text,
                       bool hovered, bool pressed = false, bool primary = true)
{
    p->save();
    const qreal radius = rect.height() / 2.0;
    QRectF r = rect.adjusted(0, pressed ? 1.5 : 0, 0, pressed ? 1.5 : 0);

    QLinearGradient g(r.topLeft(), r.bottomLeft());
    if (!primary) {
        g.setColorAt(0.0, hovered ? QColor(52, 62, 92) : QColor(36, 45, 72));
        g.setColorAt(1.0, hovered ? QColor(32, 40, 66) : QColor(24, 31, 54));
    } else if (pressed) {
        g.setColorAt(0.0, deepRed());
        g.setColorAt(1.0, chinaRed());
    } else if (hovered) {
        g.setColorAt(0.0, QColor(232, 46, 56));
        g.setColorAt(1.0, QColor(176, 18, 30));
    } else {
        g.setColorAt(0.0, QColor(198, 32, 42));
        g.setColorAt(1.0, deepRed());
    }
    p->setBrush(g);
    p->setPen(QPen(primary ? gold() : buildingEdge(), hovered ? 2.2 : 1.6));
    p->drawRoundedRect(r, radius, radius);

    p->setPen(hovered ? lightGold() : cream());
    p->drawText(r, Qt::AlignCenter, text);
    p->restore();
}

// 五角星
inline QPolygonF starPolygon(const QPointF &center, qreal outer, qreal inner,
                             qreal rotation = -M_PI / 2.0)
{
    QPolygonF poly;
    for (int i = 0; i < 10; ++i) {
        const qreal r = (i % 2 == 0) ? outer : inner;
        const qreal a = rotation + i * M_PI / 5.0;
        poly << QPointF(center.x() + std::cos(a) * r, center.y() + std::sin(a) * r);
    }
    return poly;
}

// 点是否在多边形内（射线法）
inline bool pointInPolygon(const QPointF &p, const QVector<QPointF> &poly)
{
    bool inside = false;
    const int n = poly.size();
    if (n < 3)
        return false;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        const QPointF &pi = poly[i];
        const QPointF &pj = poly[j];
        if (((pi.y() > p.y()) != (pj.y() > p.y()))
            && (p.x() < (pj.x() - pi.x()) * (p.y() - pi.y()) / (pi.y() - pj.y()) + pi.x())) {
            inside = !inside;
        }
    }
    return inside;
}

/* 可选素材加载：
 * 先找 Qt 资源（:/images/xxx），再找可执行文件同级的 images/xxx。
 * 两处都没有时返回空 QPixmap，调用方自行回退到纯矢量绘制。
 * 这样可以做到「不放照片也能跑，把照片丢进 images/ 就自动换成实景」。
 */
inline QPixmap optionalImage(const QString &fileName)
{
    QPixmap pm;
    pm.load(QStringLiteral(":/images/") + fileName);
    if (!pm.isNull())
        return pm;
    const QString diskPath = QCoreApplication::applicationDirPath()
                             + QStringLiteral("/images/") + fileName;
    pm.load(diskPath);
    return pm;
}

} // namespace Theme

#endif // THEME_H
