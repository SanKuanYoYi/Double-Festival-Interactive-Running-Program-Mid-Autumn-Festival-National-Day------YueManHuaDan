/*
 * nightscene.cpp
 * 主场景实现：软件学院夜景 + 月亮投影 + 代码粒子。
 *
 * 坐标系固定为 1280x720（逻辑像素），由 NightSceneView 做等比缩放。
 */

#include "nightscene.h"

#include "soundfx.h"

#include <QAbstractAnimation>
#include <QBrush>
#include <QFrame>
#include <QFontMetrics>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QPainterPathStroker>
#include <QPen>
#include <QPolygonF>
#include <QPropertyAnimation>
#include <QRadialGradient>
#include <QResizeEvent>

#include <cmath>

/* ========================================================================== */
/*  匿名命名空间：随机、几何、形状采样                                          */
/* ========================================================================== */
namespace {

constexpr int kShapeCount = 4;
constexpr qreal kShapeRise = 1.75;
constexpr qreal kShapeHold = 3.45;
constexpr qreal kShapeTotal = 4.30;
constexpr int kParticleCount = 420;

const QRectF kFacade(150.0, 92.0, 960.0, 472.0);
const QRectF kCornice(140.0, 80.0, 980.0, 18.0);
const QRectF kNameBand(150.0, 100.0, 960.0, 54.0);
const QRectF kEntrance(546.0, 486.0, 212.0, 78.0);
const QRectF kDeskTop(700.0, 596.0, 540.0, 76.0);
const QPointF kProjCenter(408.0, 300.0);
const qreal kProjRadius = 196.0;
const QPointF kMoonDefault(1008.0, 156.0);

struct Rng
{
    quint32 seed;
    explicit Rng(quint32 s = 20260923u) : seed(s) {}
    quint32 next() { seed = seed * 1664525u + 1013904223u; return seed; }
    qreal unit() { return qreal(next() % 100000u) / 100000.0; }
    qreal range(qreal a, qreal b) { return a + (b - a) * unit(); }
    int irange(int a, int b) { return a + int(next() % quint32(b - a + 1)); }
};

struct Area
{
    QPolygonF poly;
    int weight = 1;
};

QPolygonF polyFrom(const QVector<QPointF> &pts) { return QPolygonF(pts); }

QPolygonF polyRect(qreal x, qreal y, qreal w, qreal h)
{
    return polyFrom(QVector<QPointF>{ QPointF(x, y), QPointF(x + w, y),
                                      QPointF(x + w, y + h), QPointF(x, y + h) });
}

QPolygonF ellipsePoly(const QPointF &center, qreal rx, qreal ry)
{
    QPainterPath path;
    path.addEllipse(center, rx, ry);
    return path.toFillPolygon();
}

QPolygonF strokePoly(const QPainterPath &path, qreal width)
{
    QPainterPathStroker stroker;
    stroker.setWidth(width);
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    return stroker.createStroke(path).toFillPolygon();
}

// 注：Qt6 才提供 toFillPolygon(matrix, xform, steps) 三参数重载，
// 本工程统一走单参数版本以保证 Qt5 / Qt6 都能编译。

QVector<QPointF> sampleInside(const QPolygonF &poly, int count, Rng &rng)
{
    QVector<QPointF> out;
    out.reserve(count);
    if (poly.size() < 3)
        return out;

    const QRectF box = poly.boundingRect();
    if (box.width() <= 0.0 || box.height() <= 0.0)
        return out;

    int guard = 0;
    const int maxGuard = count * 60 + 400;
    while (out.size() < count && guard < maxGuard) {
        ++guard;
        const QPointF pt(box.left() + rng.unit() * box.width(),
                         box.top() + rng.unit() * box.height());
        if (poly.containsPoint(pt, Qt::OddEvenFill))
            out << pt;
    }
    while (out.size() < count)
        out << QPointF(box.center().x() + rng.range(-8.0, 8.0),
                       box.center().y() + rng.range(-8.0, 8.0));
    return out;
}

QVector<QPointF> buildTargets(const QVector<Area> &areas, int count, Rng &rng)
{
    QVector<QPointF> out;
    if (areas.isEmpty())
        return out;

    int totalWeight = 0;
    for (const Area &a : areas)
        totalWeight += a.weight;

    for (const Area &a : areas) {
        if (a.poly.size() < 3)
            continue;
        const int n = qMax(6, int(count * double(a.weight) / double(totalWeight)));
        out += sampleInside(a.poly, n, rng);
    }
    while (out.size() > count)
        out.removeLast();
    return out;
}

/* ---------------- 形状 0：黄河 ---------------- */
QVector<Area> areaRiver()
{
    QVector<Area> out;

    QPainterPath main;
    main.moveTo(212.0, 492.0);
    main.cubicTo(430.0, 514.0, 520.0, 404.0, 612.0, 398.0);
    main.cubicTo(724.0, 392.0, 764.0, 296.0, 884.0, 272.0);
    main.cubicTo(986.0, 250.0, 1012.0, 196.0, 1066.0, 170.0);
    out << Area{ strokePoly(main, 34.0), 5 };

    QPainterPath branch;
    branch.moveTo(612.0, 400.0);
    branch.cubicTo(566.0, 332.0, 512.0, 300.0, 468.0, 234.0);
    out << Area{ strokePoly(branch, 20.0), 3 };

    QPainterPath source;
    source.moveTo(212.0, 492.0);
    source.lineTo(190.0, 464.0);
    source.moveTo(212.0, 492.0);
    source.lineTo(240.0, 516.0);
    out << Area{ strokePoly(source, 15.0), 2 };

    return out;
}

/* ---------------- 形状 1：长城 ---------------- */
void addTicks(QPainterPath &path, const QVector<QPointF> &poly, qreal spacing, qreal height)
{
    qreal carry = 0.0;
    for (int i = 0; i + 1 < poly.size(); ++i) {
        const QPointF a = poly.at(i);
        const QPointF b = poly.at(i + 1);
        const QPointF d = b - a;
        const qreal len = std::sqrt(d.x() * d.x() + d.y() * d.y());
        if (len < 1e-6)
            continue;
        const QPointF dir(d.x() / len, d.y() / len);
        const QPointF up(dir.y(), -dir.x());
        for (qreal s = carry; s < len; s += spacing) {
            const QPointF base = a + dir * s;
            path.moveTo(base);
            path.lineTo(base + up * height);
            carry = s + spacing;
        }
        carry -= len;
    }
}

QVector<Area> areaGreatWall()
{
    const QVector<QPointF> ridge{
        QPointF(196.0, 480.0), QPointF(268.0, 414.0), QPointF(322.0, 448.0),
        QPointF(430.0, 338.0), QPointF(520.0, 406.0), QPointF(624.0, 302.0),
        QPointF(720.0, 374.0), QPointF(822.0, 298.0), QPointF(918.0, 354.0),
        QPointF(1010.0, 288.0), QPointF(1084.0, 332.0)
    };

    QVector<QPointF> wall;
    for (const QPointF &pt : ridge)
        wall << QPointF(pt.x(), pt.y() - 28.0);

    QPainterPath ridgePath;
    ridgePath.moveTo(ridge.first());
    for (int i = 1; i < ridge.size(); ++i)
        ridgePath.lineTo(ridge.at(i));

    QPainterPath wallPath;
    wallPath.moveTo(wall.first());
    for (int i = 1; i < wall.size(); ++i)
        wallPath.lineTo(wall.at(i));

    QPainterPath merlonPath;
    addTicks(merlonPath, wall, 30.0, 22.0);

    QVector<Area> out;
    out << Area{ strokePoly(wallPath, 15.0), 4 };
    out << Area{ strokePoly(ridgePath, 18.0), 4 };
    out << Area{ strokePoly(merlonPath, 8.0), 3 };
    out << Area{ polyRect(398.0, 236.0, 56.0, 104.0), 3 };   // 敌楼一
    out << Area{ polyRect(392.0, 226.0, 68.0, 12.0), 1 };
    out << Area{ polyRect(690.0, 274.0, 58.0, 102.0), 3 };   // 敌楼二
    out << Area{ polyRect(684.0, 264.0, 70.0, 12.0), 1 };
    return out;
}

/* ---------------- 形状 2：高铁 ---------------- */
QVector<Area> areaTrain()
{
    QVector<Area> out;

    const QVector<QPointF> body{
        QPointF(372.0, 400.0), QPointF(406.0, 366.0), QPointF(900.0, 366.0),
        QPointF(1002.0, 392.0), QPointF(1030.0, 422.0), QPointF(918.0, 450.0),
        QPointF(382.0, 450.0)
    };
    out << Area{ polyFrom(body), 7 };

    for (int i = 0; i < 5; ++i)
        out << Area{ polyRect(432.0 + i * 92.0, 384.0, 62.0, 26.0), 1 };

    out << Area{ ellipsePoly(QPointF(452.0, 466.0), 22.0, 22.0), 1 };
    out << Area{ ellipsePoly(QPointF(700.0, 466.0), 22.0, 22.0), 1 };
    out << Area{ ellipsePoly(QPointF(942.0, 466.0), 22.0, 22.0), 1 };

    QPainterPath rail1;
    rail1.moveTo(180.0, 492.0);
    rail1.lineTo(1096.0, 492.0);
    QPainterPath rail2;
    rail2.moveTo(180.0, 506.0);
    rail2.lineTo(1096.0, 506.0);
    out << Area{ strokePoly(rail1, 8.0), 2 };
    out << Area{ strokePoly(rail2, 8.0), 2 };

    QPainterPath wire;
    wire.moveTo(180.0, 322.0);
    wire.lineTo(520.0, 308.0);
    wire.lineTo(830.0, 320.0);
    wire.lineTo(1096.0, 304.0);
    out << Area{ strokePoly(wire, 7.0), 2 };

    QPainterPath speed;
    speed.moveTo(150.0, 372.0);
    speed.lineTo(310.0, 372.0);
    speed.moveTo(180.0, 404.0);
    speed.lineTo(360.0, 404.0);
    out << Area{ strokePoly(speed, 7.0), 1 };

    return out;
}

/* ---------------- 形状 3：航天 ---------------- */
QVector<Area> areaRocket()
{
    QVector<Area> out;

    const QVector<QPointF> body{
        QPointF(518.0, 148.0), QPointF(544.0, 234.0), QPointF(550.0, 428.0),
        QPointF(488.0, 428.0), QPointF(492.0, 234.0)
    };
    out << Area{ polyFrom(body), 6 };

    out << Area{ polyFrom(QVector<QPointF>{ QPointF(490.0, 372.0), QPointF(434.0, 452.0),
                                            QPointF(490.0, 436.0) }), 2 };
    out << Area{ polyFrom(QVector<QPointF>{ QPointF(550.0, 372.0), QPointF(606.0, 452.0),
                                            QPointF(550.0, 436.0) }), 2 };
    out << Area{ polyFrom(QVector<QPointF>{ QPointF(486.0, 428.0), QPointF(552.0, 428.0),
                                            QPointF(566.0, 468.0), QPointF(472.0, 468.0) }), 2 };
    out << Area{ polyFrom(QVector<QPointF>{ QPointF(496.0, 470.0), QPointF(519.0, 556.0),
                                            QPointF(542.0, 470.0) }), 2 };
    out << Area{ ellipsePoly(QPointF(519.0, 286.0), 24.0, 24.0), 2 };

    QPainterPath trail;
    trail.moveTo(292.0, 598.0);
    trail.cubicTo(392.0, 566.0, 470.0, 526.0, 519.0, 474.0);
    out << Area{ strokePoly(trail, 10.0), 2 };

    out << Area{ Theme::starPolygon(QPointF(300.0, 236.0), 20.0, 8.4), 1 };
    out << Area{ Theme::starPolygon(QPointF(252.0, 334.0), 13.0, 5.6), 1 };
    out << Area{ Theme::starPolygon(QPointF(772.0, 240.0), 16.0, 6.6), 1 };
    out << Area{ Theme::starPolygon(QPointF(866.0, 336.0), 11.0, 4.6), 1 };

    return out;
}

const QString &codeCharacters()
{
    static const QString chars = QStringLiteral("01{}[]()<>=+;/*&|#@$!~?:abcdefghijklmnopqrstuvwxyz");
    return chars;
}

QColor colorForProgress(qreal progress)
{
    return Theme::mixColor(Theme::codeGreen(), Theme::lightGold(), Theme::easeOutCubic(progress));
}

} // namespace

/* ========================================================================== */
/*  NightBackdrop                                                             */
/* ========================================================================== */

NightBackdrop::NightBackdrop(QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    Rng rng(7788u);

    // 星空
    for (int i = 0; i < 160; ++i) {
        Star s;
        s.pos = QPointF(rng.range(0.0, qreal(Theme::kSceneW)), rng.range(0.0, 520.0));
        s.size = rng.range(0.7, 2.1);
        s.phase = rng.range(0.0, 6.283);
        m_stars << s;
    }

    // 教学楼窗户
    m_windows = facadeWindows();
    for (int i = 0; i < m_windows.size(); ++i) {
        const qreal u = rng.unit();
        int tone = 0;
        if (u < 0.40)
            tone = 1;          // 宿舍暖黄
        else if (u < 0.53)
            tone = 2;          // 实验室冷白
        m_winTone << tone;
        m_winPhase << rng.range(0.0, 6.283);
    }

    m_moonImg.load(QStringLiteral(":/images/moon.png"));
    m_cakeImg.load(QStringLiteral(":/images/mooncake.png"));
    m_sparkImg.load(QStringLiteral(":/images/spark.png"));

    // 可选写实素材：不提供时全部走矢量绘制
    m_bgImg = Theme::optionalImage(QStringLiteral("school_night.jpg"));
    m_projImg = Theme::optionalImage(QStringLiteral("anyang_projection.jpg"));

    setFlag(QGraphicsItem::ItemIsMovable, false);
    setZValue(0.0);
}

QRectF NightBackdrop::boundingRect() const
{
    return QRectF(0.0, 0.0, qreal(Theme::kSceneW), qreal(Theme::kSceneH));
}

QVector<QRectF> NightBackdrop::facadeWindows()
{
    QVector<QRectF> out;
    const qreal x0 = 178.0, y0 = 168.0;
    const qreal w = 904.0, h = 328.0;
    const int cols = 32, rows = 10;
    const qreal cw = w / cols;
    const qreal ch = h / rows;
    const qreal ww = cw * 0.58;
    const qreal wh = ch * 0.60;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            out << QRectF(x0 + c * cw + (cw - ww) / 2.0, y0 + r * ch + (ch - wh) / 2.0, ww, wh);
    return out;
}

/* ---------------- 布局矩形 ---------------- */
QRectF NightBackdrop::moonHandleRect() const
{
    return Theme::centeredRect(m_moonPos, 148.0, 148.0);
}

QRectF NightBackdrop::mooncakeRect() const
{
    return Theme::centeredRect(QPointF(800.0, 550.0), 92.0, 92.0);
}

QRectF NightBackdrop::laptopRect() const
{
    return QRectF(882.0, 462.0, 268.0, 146.0);
}

QRectF NightBackdrop::screenRect() const
{
    return QRectF(898.0, 474.0, 226.0, 116.0);
}

QRectF NightBackdrop::startButtonRect() const
{
    return QRectF(1004.0, 632.0, 238.0, 56.0);
}

/* ---------------- 属性动画 ---------------- */
void NightBackdrop::setMoonPos(const QPointF &pos)
{
    if (qAbs(pos.x() - m_moonPos.x()) < 0.01 && qAbs(pos.y() - m_moonPos.y()) < 0.01)
        return;
    m_moonPos = pos;
    update();
    emit moonChanged();
}

void NightBackdrop::setMoonGlow(qreal glow)
{
    if (qAbs(glow - m_moonGlow) < 0.001)
        return;
    m_moonGlow = glow;
    update();
    emit moonChanged();
}

void NightBackdrop::riseMoon()
{
    setMoonPos(QPointF(kMoonDefault.x(), 430.0));
    auto *anim = new QPropertyAnimation(this, "moonPos");
    anim->setDuration(2600);
    anim->setStartValue(QVariant::fromValue(QPointF(kMoonDefault.x(), 430.0)));
    anim->setEndValue(QVariant::fromValue(kMoonDefault));
    anim->setEasingCurve(QEasingCurve(QEasingCurve::OutCubic));
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

/* ---------------- 每帧推进 ---------------- */
void NightBackdrop::tick(qreal dt)
{
    m_time += dt;
    m_pulse += dt;

    if (m_proj != NoProjection)
        m_projAlpha = qMin(1.0, m_projAlpha + dt * 2.0);
    else
        m_projAlpha = qMax(0.0, m_projAlpha - dt * 2.6);
    if (m_projAlpha > 0.0)
        m_projTime += dt;

    if (m_codeRunning) {
        m_shapeTime += dt;
        if (m_shapeTime > kShapeTotal) {
            if (m_shapeIndex >= kShapeCount - 1) {
                // 一次巡礼结束：金粉散尽
                m_codeRunning = false;
                m_shapeTime = 0.0;
                m_particles.clear();
            } else {
                startShape(m_shapeIndex + 1);
            }
        }
    }
    update();
}

void NightBackdrop::reset()
{
    m_proj = NoProjection;
    m_projAlpha = 0.0;
    m_projTime = 0.0;
    m_codeRunning = false;
    m_shapeIndex = -1;
    m_shapeTime = 0.0;
    m_particles.clear();
    m_hoverRole = HitNone;
    m_pressRole = HitNone;
    m_pressing = false;
    m_dragging = false;
    riseMoon();
    update();
}

/* ---------------- 交互 ---------------- */
int NightBackdrop::hitTest(const QPointF &scenePos) const
{
    if (startButtonRect().contains(scenePos))
        return HitStart;
    if (moonHandleRect().contains(scenePos))
        return HitMoon;
    if (mooncakeRect().contains(scenePos))
        return HitMooncake;
    if (laptopRect().contains(scenePos))
        return HitLaptop;
    return HitNone;
}

void NightBackdrop::setPointer(const QPointF &scenePos)
{
    const int role = hitTest(scenePos);
    if (role != m_hoverRole) {
        m_hoverRole = role;
        update();
    }
}

void NightBackdrop::beginDrag(const QPointF &scenePos)
{
    m_pressing = true;
    m_pressRole = hitTest(scenePos);
    if (m_pressRole == HitMoon) {
        m_dragging = true;
        m_moonPressed = true;
        m_dragOffset = scenePos - m_moonPos;
        m_switchAnchor = m_moonPos.x();
    }
    update();
}

void NightBackdrop::moveDrag(const QPointF &scenePos)
{
    if (!m_dragging)
        return;

    const QPointF target = scenePos - m_dragOffset;
    setMoonPos(QPointF(qBound(96.0, target.x(), 1184.0), qBound(96.0, target.y(), 372.0)));

    // 横向拖过一定距离就切换一次投影场景
    const qreal dx = m_moonPos.x() - m_switchAnchor;
    if (dx > 108.0) {
        const int next = (m_proj == NoProjection) ? int(AnyangHome) : (m_proj + 1) % 3;
        showProjection(next);
        m_switchAnchor = m_moonPos.x();
    } else if (dx < -108.0) {
        const int prev = (m_proj == NoProjection) ? int(CampusMemory) : (m_proj + 2) % 3;
        showProjection(prev);
        m_switchAnchor = m_moonPos.x();
    }
}

void NightBackdrop::endDrag()
{
    const int role = m_pressRole;
    m_dragging = false;
    m_moonPressed = false;
    m_pressing = false;
    m_pressRole = HitNone;
    if (role != HitNone)
        trigger(HitRole(role));
    update();
}

void NightBackdrop::trigger(HitRole role)
{
    switch (role) {
    case HitMooncake:
        if (m_proj != AnyangHome) {
            showProjection(AnyangHome);
            auto *glow = new QPropertyAnimation(this, "moonGlow");
            glow->setDuration(900);
            glow->setStartValue(m_moonGlow);
            glow->setKeyValueAt(0.5, 1.0);
            glow->setEndValue(0.9);
            glow->start(QAbstractAnimation::DeleteWhenStopped);
            SoundFx::playChime();
        }
        break;
    case HitLaptop:
        SoundFx::playChime();
        if (!m_codeRunning)
            startShape(0);
        else
            startShape((m_shapeIndex + 1) % kShapeCount);
        break;
    case HitStart:
        SoundFx::playClick();
        emit requestGame();
        break;
    default:
        break;
    }
}

void NightBackdrop::showProjection(int index)
{
    // 允许传入 NoProjection(-1) 收起投影；其余越界值忽略
    if (index < NoProjection || index > CampusMemory)
        return;
    m_proj = index;
    m_projTime = 0.0;
    update();
}

/* ---------------- 形状与粒子 ---------------- */
void NightBackdrop::ensureShapes(int index)
{
    if (index < 0 || index >= kShapeCount)
        return;
    while (m_shapeCache.size() < kShapeCount)
        m_shapeCache << QVector<QPointF>();
    if (!m_shapeCache.at(index).isEmpty())
        return;

    Rng rng(20260923u + quint32(index) * 7919u);
    switch (index) {
    case 0: m_shapeCache[index] = buildTargets(areaRiver(), kParticleCount, rng); break;
    case 1: m_shapeCache[index] = buildTargets(areaGreatWall(), kParticleCount, rng); break;
    case 2: m_shapeCache[index] = buildTargets(areaTrain(), kParticleCount, rng); break;
    default: m_shapeCache[index] = buildTargets(areaRocket(), kParticleCount, rng); break;
    }
}

const QVector<QPointF> &NightBackdrop::shapeTargets(int index) const
{
    if (index < 0 || index >= m_shapeCache.size())
        return m_empty;
    return m_shapeCache.at(index);
}

void NightBackdrop::startShape(int index)
{
    ensureShapes(index);
    if (shapeTargets(index).isEmpty())
        return;
    m_shapeIndex = index;
    m_shapeTime = 0.0;
    m_codeRunning = true;
    rebuildParticles();

    auto *glow = new QPropertyAnimation(this, "moonGlow");
    glow->setDuration(1200);
    glow->setStartValue(m_moonGlow);
    glow->setKeyValueAt(0.5, 1.0);
    glow->setEndValue(0.86);
    glow->start(QAbstractAnimation::DeleteWhenStopped);
    update();
}

void NightBackdrop::rebuildParticles()
{
    const QVector<QPointF> &targets = shapeTargets(m_shapeIndex);
    m_particles.clear();
    if (targets.isEmpty())
        return;

    Rng rng(20260923u + quint32(m_shapeIndex) * 104729u);
    const QRectF screen = screenRect();
    const QString chars = codeCharacters();

    m_particles.reserve(kParticleCount);
    for (int i = 0; i < kParticleCount; ++i) {
        Particle pt;
        pt.from = QPointF(screen.center().x() + rng.range(-150.0, 150.0),
                          screen.center().y() + rng.range(-96.0, 96.0));
        pt.to = targets.at(rng.irange(0, targets.size() - 1));
        pt.ch = chars.at(rng.irange(0, chars.size() - 1));
        pt.delay = rng.range(0.0, 0.55);
        pt.dur = rng.range(0.85, 1.35);
        pt.size = rng.range(9.0, 16.0);
        pt.wobble = rng.range(0.0, 6.283);
        m_particles << pt;
    }
}

/* ========================================================================== */
/*  绘制                                                                       */
/* ========================================================================== */
void NightBackdrop::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing, true);
    if (m_bgImg.isNull()) {
        drawSky(painter);
        drawStars(painter);
        drawRemoteCity(painter);
        drawCampusBuilding(painter);
        drawGround(painter);
    } else {
        // 有写实底图时，天空/楼体/地面交给照片，
        // 月亮、月饼、电脑、投影、粒子这些交互元素仍然画在照片之上
        drawPhotoBackdrop(painter);
    }
    drawProjection(painter);
    drawDesk(painter);
    drawMoon(painter);
    drawCodeParticles(painter);
    drawHud(painter);
}

/* ---------------- 写实底图（可选素材） ---------------- */
void NightBackdrop::drawPhotoBackdrop(QPainter *p) const
{
    if (m_bgImg.isNull())
        return;

    const QRectF target(0.0, 0.0, qreal(Theme::kSceneW), qreal(Theme::kSceneH));

    // 等比裁剪铺满，不拉伸变形
    const qreal sw = qreal(m_bgImg.width());
    const qreal sh = qreal(m_bgImg.height());
    const qreal targetRatio = target.width() / target.height();
    QRectF src;
    if (sw / sh > targetRatio) {
        const qreal w = sh * targetRatio;
        src = QRectF((sw - w) / 2.0, 0.0, w, sh);
    } else {
        const qreal h = sw / targetRatio;
        src = QRectF(0.0, (sh - h) / 2.0, sw, h);
    }

    p->save();
    p->drawPixmap(target, m_bgImg, src);

    // 夜间压暗 + 上下渐隐，保证叠上去的矢量元素和 HUD 依旧清晰
    QLinearGradient veil(target.topLeft(), target.bottomLeft());
    veil.setColorAt(0.0, QColor(6, 9, 22, 168));
    veil.setColorAt(0.45, QColor(6, 9, 22, 96));
    veil.setColorAt(1.0, QColor(6, 9, 22, 182));
    p->fillRect(target, veil);
    p->restore();
}

void NightBackdrop::drawSky(QPainter *p) const
{
    QLinearGradient sky(0.0, 0.0, 0.0, qreal(Theme::kSceneH));
    sky.setColorAt(0.0, Theme::nightTop());
    sky.setColorAt(0.52, QColor(14, 20, 42));
    sky.setColorAt(1.0, Theme::nightBottom());
    p->setPen(Qt::NoPen);
    p->setBrush(sky);
    p->drawRect(boundingRect());

    // 月亮周围的暖大气光
    QRadialGradient halo(m_moonPos, 460.0);
    halo.setColorAt(0.0, Theme::withAlpha(Theme::lightGold(), int(46.0 * m_moonGlow)));
    halo.setColorAt(0.45, Theme::withAlpha(Theme::gold(), int(16.0 * m_moonGlow)));
    halo.setColorAt(1.0, Theme::withAlpha(Theme::gold(), 0));
    p->setBrush(halo);
    p->drawRect(boundingRect());
}

void NightBackdrop::drawStars(QPainter *p) const
{
    p->setPen(Qt::NoPen);
    for (const Star &s : m_stars) {
        const qreal twinkle = 0.45 + 0.55 * std::sin(m_time * 1.4 + s.phase);
        p->setBrush(Theme::withAlpha(Theme::warmWhite(), int(210.0 * twinkle)));
        p->drawEllipse(s.pos, s.size, s.size);
    }
}

void NightBackdrop::drawRemoteCity(QPainter *p) const
{
    Rng rng(424242u);
    p->setPen(Qt::NoPen);
    for (int i = 0; i < 26; ++i) {
        const qreal w = rng.range(34.0, 78.0);
        const qreal h = rng.range(60.0, 190.0);
        const qreal x = i * 52.0 + rng.range(-10.0, 10.0);
        const QRectF rect(x, 560.0 - h, w, h);
        p->setBrush(Theme::mixColor(Theme::building(), Theme::nightBottom(), 0.45));
        p->drawRect(rect);
        // 零星亮灯
        for (int k = 0; k < 8; ++k) {
            if (rng.unit() < 0.45)
                continue;
            const QPointF pt(rect.left() + rng.range(6.0, rect.width() - 8.0),
                             rect.top() + rng.range(6.0, rect.height() - 8.0));
            p->setBrush(Theme::withAlpha(Theme::windowLit(), int(120.0 + 60.0 * std::sin(m_time * 1.1 + k))));
            p->drawRect(QRectF(pt.x(), pt.y(), 4.0, 5.0));
        }
    }
    // 地平线雾气
    QLinearGradient fog(0.0, 480.0, 0.0, 600.0);
    fog.setColorAt(0.0, Theme::withAlpha(Theme::nightBottom(), 0));
    fog.setColorAt(1.0, Theme::withAlpha(Theme::nightBottom(), 210));
    p->setBrush(fog);
    p->drawRect(QRectF(0.0, 480.0, qreal(Theme::kSceneW), 120.0));
}

void NightBackdrop::drawCampusBuilding(QPainter *p) const
{
    p->setPen(Qt::NoPen);

    // 楼体
    p->setBrush(Theme::building());
    p->drawRect(kFacade);

    // 侧面柱 fissure lines
    p->setBrush(QColor(18, 24, 44));
    for (int i = 1; i < 6; ++i)
        p->drawRect(QRectF(kFacade.left() + i * 160.0, kFacade.top(), 3.0, kFacade.height()));

    // 檐口
    p->setBrush(QColor(20, 26, 46));
    p->drawRect(kCornice);
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 70), 1.2));
    p->drawLine(kCornice.bottomLeft(), kCornice.bottomRight());
    p->setPen(Qt::NoPen);

    // 名牌
    p->setBrush(QColor(24, 31, 56));
    p->drawRect(kNameBand);
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 90), 1.4));
    p->drawRect(kNameBand);
    p->setPen(Qt::NoPen);

    p->setFont(Theme::font(30, true));
    Theme::drawGlowText(p, QPointF(kFacade.center().x(), 126.0),
                        QStringLiteral("软 件 学 院"), Theme::cream(), Theme::gold(), 5.0);
    p->setFont(Theme::font(12));
    p->setPen(Theme::withAlpha(Theme::cream(), 150));
    p->drawText(QRectF(kFacade.center().x() - 200.0, 138.0, 400.0, 18.0), Qt::AlignCenter,
                QStringLiteral("SCHOOL OF SOFTWARE · 留校团圆"));

    // 窗户：先画光晕，再画玻璃，避免互相压暗
    p->setBrush(Theme::glassDark());
    for (const QRectF &w : m_windows)
        p->drawRect(w);

    for (int i = 0; i < m_windows.size(); ++i) {
        const int tone = m_winTone.at(i);
        if (tone == 0)
            continue;
        const QRectF &w = m_windows.at(i);
        const QColor base = (tone == 1) ? Theme::windowLit() : QColor(196, 226, 255);
        const qreal flick = 0.84 + 0.16 * std::sin(m_time * 1.7 + m_winPhase.at(i));
        p->setBrush(Theme::withAlpha(base, int(40.0 * flick)));
        p->drawRect(w.adjusted(-5.0, -6.0, 5.0, 6.0));
    }
    for (int i = 0; i < m_windows.size(); ++i) {
        const int tone = m_winTone.at(i);
        const QRectF &w = m_windows.at(i);
        if (tone == 0)
            continue;
        const QColor base = (tone == 1) ? Theme::windowLit() : QColor(196, 226, 255);
        const qreal flick = 0.84 + 0.16 * std::sin(m_time * 1.7 + m_winPhase.at(i));
        p->setBrush(Theme::withAlpha(base, int(238.0 * flick)));
        p->drawRect(w);
    }

    // 入口
    p->setBrush(QColor(30, 38, 62));
    p->drawRect(kEntrance);
    p->setBrush(Theme::withAlpha(Theme::windowLit(), 150));
    p->drawRect(QRectF(kEntrance.left() + 42.0, kEntrance.top() + 22.0, 128.0, kEntrance.height() - 22.0));
    // 门厅光洒地
    QLinearGradient spill(0.0, kEntrance.bottom(), 0.0, kEntrance.bottom() + 46.0);
    spill.setColorAt(0.0, Theme::withAlpha(Theme::windowLit(), 90));
    spill.setColorAt(1.0, Theme::withAlpha(Theme::windowLit(), 0));
    p->setBrush(spill);
    p->drawPolygon(QVector<QPointF>{ QPointF(kEntrance.left() + 30.0, kEntrance.bottom()),
                                     QPointF(kEntrance.right() - 30.0, kEntrance.bottom()),
                                     QPointF(700.0, kEntrance.bottom() + 46.0),
                                     QPointF(604.0, kEntrance.bottom() + 46.0) });

    // 基座
    p->setBrush(QColor(16, 21, 38));
    p->drawRect(QRectF(kFacade.left() - 10.0, kFacade.bottom(), kFacade.width() + 20.0, 34.0));
}

void NightBackdrop::drawGround(QPainter *p) const
{
    QLinearGradient g(0.0, 596.0, 0.0, qreal(Theme::kSceneH));
    g.setColorAt(0.0, QColor(17, 23, 42));
    g.setColorAt(1.0, QColor(8, 11, 22));
    p->setPen(Qt::NoPen);
    p->setBrush(g);
    p->drawRect(QRectF(0.0, 596.0, qreal(Theme::kSceneW), qreal(Theme::kSceneH) - 596.0));

    // 湿地面上的暖色倒影
    p->setCompositionMode(QPainter::CompositionMode_Plus);
    for (int i = 0; i < 14; ++i) {
        const qreal x = 190.0 + i * 62.0;
        const qreal h = 40.0 + 26.0 * std::sin(m_time * 0.8 + i);
        QLinearGradient rg(x, 598.0, x, 598.0 + h);
        rg.setColorAt(0.0, Theme::withAlpha(Theme::windowLit(), 34));
        rg.setColorAt(1.0, Theme::withAlpha(Theme::windowLit(), 0));
        p->setBrush(rg);
        p->drawRect(QRectF(x - 9.0, 598.0, 18.0, h));
    }
    p->setCompositionMode(QPainter::CompositionMode_SourceOver);

    // 路灯
    for (const qreal x : { 96.0, 1188.0 }) {
        p->setPen(QPen(QColor(38, 46, 70), 4.0));
        p->drawLine(QPointF(x, 640.0), QPointF(x, 560.0));
        p->setPen(Qt::NoPen);
        QRadialGradient lamp(x, 552.0, 90.0);
        lamp.setColorAt(0.0, Theme::withAlpha(Theme::lightGold(), 170));
        lamp.setColorAt(1.0, Theme::withAlpha(Theme::lightGold(), 0));
        p->setBrush(lamp);
        p->drawEllipse(QPointF(x, 552.0), 90.0, 90.0);
        p->setBrush(Theme::cream());
        p->drawEllipse(QPointF(x, 552.0), 9.0, 7.0);
    }

    // 前景树影
    paintTree(p, QPointF(36.0, 700.0), 300.0, Theme::withAlpha(QColor(6, 9, 18), 235));
    paintTree(p, QPointF(1256.0, 706.0), 330.0, Theme::withAlpha(QColor(6, 9, 18), 235));
}

void NightBackdrop::drawDesk(QPainter *p) const
{
    // 桌面
    QLinearGradient deskGrad(kDeskTop.topLeft(), kDeskTop.bottomLeft());
    deskGrad.setColorAt(0.0, QColor(64, 42, 30));
    deskGrad.setColorAt(0.35, QColor(48, 31, 23));
    deskGrad.setColorAt(1.0, QColor(28, 18, 14));
    p->setPen(Qt::NoPen);
    p->setBrush(deskGrad);
    p->drawRect(kDeskTop);
    p->setBrush(Theme::withAlpha(Theme::gold(), 40));
    p->drawRect(QRectF(kDeskTop.left(), kDeskTop.top(), kDeskTop.width(), 2.0));

    const bool hoverCake = (m_hoverRole == HitMooncake);
    const bool hoverLaptop = (m_hoverRole == HitLaptop);

    /* ---- 月饼与盘子 ---- */
    p->setBrush(QColor(212, 214, 222));
    p->drawEllipse(QPointF(802.0, 590.0), 92.0, 24.0);
    p->setBrush(QColor(168, 172, 184));
    p->drawEllipse(QPointF(802.0, 594.0), 78.0, 18.0);

    const QRectF cake2 = Theme::centeredRect(QPointF(744.0, 566.0), 66.0, 66.0);
    if (!m_cakeImg.isNull())
        p->drawPixmap(cake2.toRect(), m_cakeImg);
    else {
        p->setBrush(QColor(190, 130, 52));
        p->drawEllipse(cake2);
    }

    const qreal lift = hoverCake ? 6.0 + 1.6 * std::sin(m_time * 6.0) : 0.0;
    const QRectF cake = mooncakeRect().adjusted(10.0, 10.0 - lift, -10.0, -10.0 - lift);
    if (!m_cakeImg.isNull())
        p->drawPixmap(cake.toRect(), m_cakeImg);
    else {
        p->setBrush(QColor(206, 142, 56));
        p->drawEllipse(cake);
    }
    if (hoverCake) {
        p->setPen(QPen(Theme::withAlpha(Theme::gold(), 160), 2.0, Qt::DashLine));
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(mooncakeRect().adjusted(-6.0, -6.0, 6.0, 6.0));
        p->setPen(Qt::NoPen);
    }

    /* ---- 笔记本电脑 ---- */
    const QRectF laptop = laptopRect();
    const qreal laptopLift = hoverLaptop ? 2.0 : 0.0;
    p->setBrush(QColor(44, 48, 62));
    p->drawRect(QRectF(886.0, 464.0 - laptopLift, 262.0, 130.0));
    p->setBrush(QColor(58, 62, 78));
    p->drawRect(QRectF(886.0, 464.0 - laptopLift, 262.0, 8.0));

    const QRectF screen = screenRect().adjusted(0.0, -laptopLift, 0.0, -laptopLift);
    p->setBrush(QColor(9, 13, 24));
    p->drawRect(screen);

    // 屏幕里的代码
    p->save();
    p->setClipRect(screen.adjusted(2.0, 2.0, -2.0, -2.0));
    p->setFont(Theme::font(12));
    const QStringList lines{
        QStringLiteral("#include <moon.h>"),
        QStringLiteral("while (still_on_campus) {"),
        QStringLiteral("    send(mooncake, home);"),
        QStringLiteral("    draw(family, Anyang);"),
        QStringLiteral("    wish(country, Birthday77);"),
        QStringLiteral("    lit_window(me);"),
        QStringLiteral("}")
    };
    const int visible = int(m_time * 2.2);
    for (int i = 0; i < lines.size() && i <= visible; ++i) {
        const bool last = (i == visible);
        p->setPen(Theme::withAlpha(i % 2 == 0 ? Theme::codeGreen() : Theme::gold(), last ? 240 : 170));
        p->drawText(QPointF(screen.left() + 12.0, screen.top() + 22.0 + i * 16.0), lines.at(i));
    }
    if (visible % 2 == 0) {
        p->setBrush(Theme::codeGreen());
        p->drawRect(QRectF(screen.left() + 12.0, screen.top() + 22.0 + (visible % lines.size()) * 16.0 - 10.0, 7.0, 12.0));
    }
    p->restore();

    // 屏幕外溢的光
    QRadialGradient screenGlow(screen.center(), 210.0);
    screenGlow.setColorAt(0.0, Theme::withAlpha(Theme::codeGreen(), int(26.0 + 10.0 * std::sin(m_time * 2.0))));
    screenGlow.setColorAt(1.0, Theme::withAlpha(Theme::codeGreen(), 0));
    p->setBrush(screenGlow);
    p->drawRect(QRectF(screen.center() - QPointF(210.0, 210.0), QSizeF(420.0, 420.0)));

    if (hoverLaptop) {
        p->setPen(QPen(Theme::withAlpha(Theme::codeGreen(), 170), 2.0, Qt::DashLine));
        p->setBrush(Qt::NoBrush);
        p->drawRoundedRect(laptop.adjusted(-6.0, -6.0, 6.0, 6.0), 8.0, 8.0);
        p->setPen(Qt::NoPen);
    }
    Q_UNUSED(laptop)
}

void NightBackdrop::drawMoon(QPainter *p) const
{
    p->setRenderHint(QPainter::Antialiasing, true);
    const qreal radius = 62.0 + 2.0 * std::sin(m_time * 0.9);

    // 外层光晕
    QRadialGradient halo(m_moonPos, radius * 3.4);
    halo.setColorAt(0.0, Theme::withAlpha(Theme::lightGold(), int(72.0 * m_moonGlow)));
    halo.setColorAt(0.35, Theme::withAlpha(Theme::gold(), int(26.0 * m_moonGlow)));
    halo.setColorAt(1.0, Theme::withAlpha(Theme::gold(), 0));
    p->setPen(Qt::NoPen);
    p->setBrush(halo);
    p->drawEllipse(m_moonPos, radius * 3.4, radius * 3.4);

    if (!m_moonImg.isNull()) {
        p->drawPixmap(QRectF(m_moonPos.x() - radius * 1.9, m_moonPos.y() - radius * 1.9,
                             radius * 3.8, radius * 3.8).toRect(), m_moonImg);
    } else {
        QRadialGradient body(QPointF(m_moonPos.x() - radius * 0.3, m_moonPos.y() - radius * 0.3), radius * 1.6);
        body.setColorAt(0.0, Theme::cream());
        body.setColorAt(0.75, Theme::mixColor(Theme::cream(), Theme::gold(), 0.45));
        body.setColorAt(1.0, Theme::mixColor(Theme::gold(), Theme::cream(), 0.3));
        p->setBrush(body);
        p->drawEllipse(m_moonPos, radius, radius);
    }

    // 中秋月在农历十五：满月
    if (m_hoverRole == HitMoon || m_dragging) {
        p->setPen(QPen(Theme::withAlpha(Theme::lightGold(), 140), 1.6, Qt::DashLine));
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(m_moonPos, radius * 1.35, radius * 1.35);
        p->setPen(Qt::NoPen);
        p->setFont(Theme::font(13));
        p->setPen(Theme::withAlpha(Theme::cream(), 180));
        p->drawText(QRectF(m_moonPos.x() - 90.0, m_moonPos.y() + radius * 1.5, 180.0, 20.0),
                    Qt::AlignCenter, QStringLiteral("← 拖我切换投影 →"));
    }
}

/* ---------------- 投影 ---------------- */
void NightBackdrop::drawProjection(QPainter *p) const
{
    if (m_projAlpha <= 0.001 || m_proj < 0)
        return;

    const qreal alpha = Theme::easeOutCubic(m_projAlpha);
    const QPointF center = kProjCenter;
    const qreal radius = kProjRadius;

    p->save();
    p->setOpacity(alpha);

    // 投影光束：月亮 -> 圆幕
    const QPointF dir = center - m_moonPos;
    const qreal len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    if (len > 1.0) {
        const QPointF unit(dir.x() / len, dir.y() / len);
        const QPointF up(-unit.y(), unit.x());
        QLinearGradient beam(m_moonPos, center);
        beam.setColorAt(0.0, Theme::withAlpha(Theme::lightGold(), 78));
        beam.setColorAt(0.45, Theme::withAlpha(Theme::gold(), 34));
        beam.setColorAt(1.0, Theme::withAlpha(Theme::gold(), 14));
        p->setPen(Qt::NoPen);
        p->setBrush(beam);
        p->drawPolygon(QVector<QPointF>{
            m_moonPos + up * 26.0,
            center + up * radius * 0.98,
            center - up * radius * 0.98,
            m_moonPos - up * 26.0
        });
    }

    // 圆幕
    QRadialGradient disc(center, radius);
    disc.setColorAt(0.0, Theme::withAlpha(QColor(24, 30, 54), 190));
    disc.setColorAt(0.72, Theme::withAlpha(QColor(18, 24, 46), 205));
    disc.setColorAt(1.0, Theme::withAlpha(Theme::gold(), 60));
    p->setBrush(disc);
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 120), 1.6));
    p->drawEllipse(center, radius, radius);

    // 幕布里的画面
    p->setClipRect(QRectF(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0));

    const QColor line = Theme::withAlpha(Theme::lightGold(), 235);
    const qreal drawW = radius * 2.0;
    const qreal drawH = radius * 2.0;
    const QRectF stage(center.x() - radius, center.y() - radius, drawW, drawH);

    if (!m_projImg.isNull()) {
        // 有 anyang_projection.jpg 时，直接把实景贴进圆幕
        QPainterPath clip;
        clip.addEllipse(center, radius, radius);
        p->save();
        p->setClipPath(clip);
        p->setOpacity(alpha * 0.96);
        // 用三参数版本（目标矩形 / 图 / 源矩形），Qt5、Qt6 都支持
        p->drawPixmap(QRectF(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0),
                      m_projImg, QRectF(m_projImg.rect()));
        p->restore();
        // 罩一层夜色，让照片融进整幅画面
        p->setPen(Qt::NoPen);
        p->setBrush(Theme::withAlpha(QColor(10, 14, 30), 108));
        p->drawEllipse(center, radius, radius);
    } else {
        switch (m_proj) {
        case AnyangHome:
            paintWenfengTower(p, QRectF(stage.left() + 8.0, stage.top() + 30.0, 168.0, 288.0), line);
            paintOracleGlyph(p, QRectF(stage.right() - 176.0, stage.top() + 44.0, 150.0, 150.0), 0, line);
            paintYard(p, QRectF(stage.left() + 168.0, stage.bottom() - 148.0, 230.0, 140.0), line);
            break;
        case OracleGuo:
            paintOracleGlyph(p, QRectF(center.x() - 132.0, center.y() - 104.0, 264.0, 264.0), 1, line);
            paintFiveStars(p, QRectF(center.x() - 90.0, stage.top() + 26.0, 180.0, 62.0), line);
            break;
        default:
            paintCampusTower(p, QRectF(stage.left() + 42.0, stage.top() + 34.0, 190.0, 300.0), line);
            paintBike(p, QPointF(stage.right() - 132.0, stage.bottom() - 60.0), 1.05, line);
            paintBike(p, QPointF(stage.right() - 62.0, stage.bottom() - 44.0), 0.85, line);
            paintFigure(p, QPointF(center.x() - 30.0, stage.bottom() - 22.0), 74.0,
                        Theme::withAlpha(line, 170));
            paintFigure(p, QPointF(center.x() - 2.0, stage.bottom() - 22.0), 66.0,
                        Theme::withAlpha(line, 140));
            break;
        }
    }
    p->setClipping(false);

    // 标题
    p->setOpacity(alpha);
    QString title;
    switch (m_proj) {
    case AnyangHome: title = QStringLiteral("小家团圆 · 安阳"); break;
    case OracleGuo: title = QStringLiteral("家国同庆 · 甲骨"); break;
    default: title = QStringLiteral("校园团圆 · 软件学院"); break;
    }
    p->setFont(Theme::font(23, true));
    Theme::drawGlowText(p, QPointF(center.x(), center.y() + radius + 38.0),
                        title, Theme::cream(), Theme::gold(), 6.0);
    p->setFont(Theme::font(12));
    p->setPen(Theme::withAlpha(Theme::cream(), 150));
    p->drawText(QRectF(center.x() - radius, center.y() + radius + 58.0, radius * 2.0, 18.0),
                Qt::AlignCenter, QStringLiteral("拖动月亮可切换投影场景"));
    p->restore();
}

/* ---------------- 代码粒子 ---------------- */
void NightBackdrop::drawCodeParticles(QPainter *p) const
{
    if (!m_codeRunning || m_particles.isEmpty())
        return;

    p->save();
    p->setRenderHint(QPainter::TextAntialiasing, true);

    const bool scattering = m_shapeTime > kShapeHold;
    for (int i = 0; i < m_particles.size(); ++i) {
        const Particle &pt = m_particles.at(i);
        const qreal raw = (m_shapeTime - pt.delay) / pt.dur;
        const qreal t = scattering ? 1.0 : Theme::clamp01(raw);
        if (raw <= 0.0)
            continue;

        const qreal e = Theme::easeOutCubic(t);
        QPointF pos = Theme::lerpPoint(pt.from, pt.to, e);
        const qreal swirl = (1.0 - e) * 26.0;
        pos += QPointF(std::sin(m_time * 1.6 + pt.wobble) * swirl,
                       std::cos(m_time * 1.3 + pt.wobble * 1.7) * swirl * 0.6);

        qreal fade = 1.0;
        if (scattering) {
            const qreal s = Theme::clamp01((m_shapeTime - kShapeHold) / (kShapeTotal - kShapeHold));
            const QPointF outward(std::cos(pt.wobble * 3.1), std::sin(pt.wobble * 2.3));
            pos += outward * 150.0 * Theme::easeOutCubic(s);
            fade = 1.0 - s;
        } else if (raw < 1.0) {
            fade = Theme::clamp01(raw * 2.2);
        }
        if (fade <= 0.02)
            continue;

        const QColor color = Theme::withAlpha(colorForProgress(e), int(235.0 * fade));
        QFont f(QString::fromUtf8("Consolas"), -1, QFont::Normal);
        f.setPixelSize(int(pt.size + 2.0 * e));
        p->setFont(f);
        p->setPen(color);
        p->drawText(pos, QString(pt.ch));

        // 每 4 个粒子补一颗星火，保证帧率
        if (!m_sparkImg.isNull() && (i % 4 == 0)) {
            const qreal s = pt.size * 0.9;
            p->setOpacity(0.55 * fade);
            p->drawPixmap(QRectF(pos.x() - s, pos.y() - s, s * 2.0, s * 2.0).toRect(), m_sparkImg);
            p->setOpacity(1.0);
        }
    }
    p->restore();

    // 底部字幕
    QString caption;
    switch (m_shapeIndex) {
    case 0: caption = QStringLiteral("黄河奔涌 · 山河锦绣"); break;
    case 1: caption = QStringLiteral("长城巍峨 · 国泰民安"); break;
    case 2: caption = QStringLiteral("高铁飞驰 · 日新月异"); break;
    case 3: caption = QStringLiteral("航天逐梦 · 星辰大海"); break;
    default: caption.clear(); break;
    }
    if (!caption.isEmpty()) {
        const qreal appear = Theme::easeOutCubic(Theme::clamp01(m_shapeTime / 0.6));
        p->setOpacity(appear * Theme::clamp01(1.2 - (m_shapeTime - kShapeHold)));
        p->setFont(Theme::font(26, true));
        Theme::drawGlowText(p, QPointF(qreal(Theme::kSceneW) / 2.0, 596.0), caption,
                            Theme::cream(), Theme::gold(), 7.0);
        p->setOpacity(1.0);
    }
}

/* ---------------- HUD ---------------- */
void NightBackdrop::drawHud(QPainter *p) const
{
    // 标题
    p->setFont(Theme::font(25, true));
    Theme::drawGlowText(p, QPointF(150.0, 44.0), QStringLiteral("月满华诞 · 码上团圆"),
                        Theme::cream(), Theme::gold(), 6.0);
    p->setFont(Theme::font(12));
    p->setPen(Theme::withAlpha(Theme::cream(), 145));
    p->drawText(QRectF(24.0, 60.0, 320.0, 18.0), Qt::AlignLeft,
                QStringLiteral("把小家的思念，写进大家的代码里"));

    // 操作提示气泡
    auto bubble = [&](const QPointF &anchor, const QString &text, bool visible, const QColor &accent) {
        if (!visible)
            return;
        QFontMetrics fm(p->font());
        const qreal w = fm.horizontalAdvance(text) + 30.0;
        const QRectF box(anchor.x() - w / 2.0, anchor.y(), w, 32.0);
        const qreal pulse = 0.82 + 0.18 * std::sin(m_time * 3.0);
        p->setPen(QPen(Theme::withAlpha(accent, int(150.0 * pulse)), 1.4));
        p->setBrush(Theme::withAlpha(QColor(14, 20, 38), 205));
        p->drawRoundedRect(box, 16.0, 16.0);
        p->setPen(Theme::withAlpha(accent, 230));
        p->drawText(box, Qt::AlignCenter, text);
        p->setPen(QPen(Theme::withAlpha(accent, int(150.0 * pulse)), 1.4));
        p->drawLine(QPointF(box.center().x(), box.bottom()),
                    QPointF(box.center().x(), box.bottom() + 12.0));
    };

    p->setFont(Theme::font(13));
    bubble(QPointF(800.0, 452.0), QStringLiteral("点我 · 投影家乡"),
           m_proj == NoProjection, Theme::gold());
    bubble(QPointF(1016.0, 404.0), QStringLiteral("点我 · 用代码贺华诞"),
           !m_codeRunning, Theme::codeGreen());

    // 左下操作说明
    p->setPen(Qt::NoPen);
    p->setBrush(Theme::withAlpha(QColor(10, 14, 28), 170));
    p->drawRoundedRect(QRectF(24.0, 636.0, 470.0, 58.0), 12.0, 12.0);
    p->setFont(Theme::font(12));
    p->setPen(Theme::withAlpha(Theme::cream(), 190));
    p->drawText(QRectF(40.0, 646.0, 450.0, 20.0), Qt::AlignLeft,
                QStringLiteral("拖动月亮：切换投影   点击月饼：安阳入梦   点击电脑：代码献礼"));
    p->setPen(Theme::withAlpha(Theme::cream(), 120));
    p->drawText(QRectF(40.0, 668.0, 450.0, 20.0), Qt::AlignLeft,
                QStringLiteral("献给每一位留校过双节的同学 · Esc 返回首页"));

    // 右下开始按钮
    Theme::drawButton(p, startButtonRect(), QStringLiteral("接月饼 · 点亮全楼 →"),
                      m_hoverRole == HitStart, m_pressing && m_pressRole == HitStart);
}

/* ========================================================================== */
/*  可复用素材绘制                                                             */
/* ========================================================================== */
void NightBackdrop::paintWenfengTower(QPainter *p, const QRectF &rect, const QColor &line)
{
    // 安阳文峰塔（天宁寺塔）：五级密檐、上宽下窄，塔刹为喇嘛塔式
    const qreal cx = rect.center().x();
    const qreal bottom = rect.bottom();
    const qreal step = rect.height() * 0.155;
    const qreal baseW = rect.width() * 0.30;
    const qreal topW = rect.width() * 0.50;

    p->save();
    p->setBrush(Qt::NoBrush);

    QPainterPath path;
    for (int i = 0; i < 5; ++i) {
        const qreal t0 = qreal(i) / 5.0;
        const qreal t1 = qreal(i + 1) / 5.0;
        const qreal y1 = bottom - i * step;
        const qreal y0 = y1 - step * 0.76;
        const qreal wb = baseW + (topW - baseW) * t0;
        const qreal wt = baseW + (topW - baseW) * t1;

        path.moveTo(cx - wb / 2.0, y1);
        path.lineTo(cx + wb / 2.0, y1);
        path.lineTo(cx + wt / 2.0, y0);
        path.lineTo(cx - wt / 2.0, y0);
        path.closeSubpath();

        const qreal ew = wt * 1.42;
        path.moveTo(cx - ew / 2.0, y0);
        path.lineTo(cx - ew * 0.40, y0 - step * 0.12);
        path.lineTo(cx + ew * 0.40, y0 - step * 0.12);
        path.lineTo(cx + ew / 2.0, y0);

        for (int k = -1; k <= 1; k += 2) {
            const qreal wx = cx + k * wt * 0.26;
            path.moveTo(wx, y0 + step * 0.14);
            path.lineTo(wx, y0 + step * 0.44);
        }
    }

    const qreal topY = bottom - 5.0 * step;
    path.moveTo(cx - topW * 0.20, topY);
    path.lineTo(cx - topW * 0.16, topY - rect.height() * 0.07);
    path.lineTo(cx + topW * 0.16, topY - rect.height() * 0.07);
    path.lineTo(cx + topW * 0.20, topY);
    path.addEllipse(QPointF(cx, topY - rect.height() * 0.11), rect.width() * 0.075, rect.width() * 0.075);
    path.moveTo(cx, topY - rect.height() * 0.11 - rect.width() * 0.075);
    path.lineTo(cx, topY - rect.height() * 0.20);

    p->setPen(QPen(Theme::withAlpha(line, 46), qMax(3.0, rect.width() * 0.035),
                   Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->drawPath(path);
    p->setPen(QPen(line, qMax(1.2, rect.width() * 0.012), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->drawPath(path);
    p->restore();
}

void NightBackdrop::paintOracleGlyph(QPainter *p, const QRectF &rect, int glyph, const QColor &line)
{
    const qreal s = qMin(rect.width(), rect.height());
    const QPointF c = rect.center();
    auto unitToScene = [&](qreal ux, qreal uy) {
        return QPointF(c.x() + ux * s, c.y() + uy * s);
    };

    QVector<QVector<QPointF>> strokes;
    if (glyph == 0) {
        // 甲骨文「家」：宀（屋宇）+ 豕（豕置屋下）
        strokes << QVector<QPointF>{ QPointF(-0.44, -0.18), QPointF(0.0, -0.48), QPointF(0.44, -0.18) };
        strokes << QVector<QPointF>{ QPointF(-0.44, -0.18), QPointF(-0.44, -0.04) };
        strokes << QVector<QPointF>{ QPointF(0.44, -0.18), QPointF(0.44, -0.04) };
        strokes << QVector<QPointF>{ QPointF(-0.06, -0.10), QPointF(0.02, 0.34) };
        strokes << QVector<QPointF>{ QPointF(-0.32, -0.02), QPointF(-0.20, 0.18), QPointF(0.02, 0.34) };
        strokes << QVector<QPointF>{ QPointF(-0.26, 0.08), QPointF(0.06, 0.12) };
        strokes << QVector<QPointF>{ QPointF(-0.24, 0.20), QPointF(0.06, 0.24) };
        strokes << QVector<QPointF>{ QPointF(0.02, 0.34), QPointF(-0.16, 0.46) };
        strokes << QVector<QPointF>{ QPointF(-0.16, 0.46), QPointF(-0.02, 0.40) };
        strokes << QVector<QPointF>{ QPointF(0.02, 0.34), QPointF(0.18, 0.46) };
    } else {
        // 甲骨文「国」：囗（城邑）+ 中间守护的笔画
        strokes << QVector<QPointF>{ QPointF(-0.44, -0.42), QPointF(0.44, -0.42),
                                     QPointF(0.44, 0.42), QPointF(-0.44, 0.42), QPointF(-0.44, -0.42) };
        strokes << QVector<QPointF>{ QPointF(-0.02, -0.30), QPointF(-0.02, 0.32) };
        strokes << QVector<QPointF>{ QPointF(-0.28, -0.16), QPointF(0.24, -0.16) };
        strokes << QVector<QPointF>{ QPointF(-0.24, 0.06), QPointF(0.22, 0.06) };
        strokes << QVector<QPointF>{ QPointF(-0.20, 0.28), QPointF(0.20, 0.28) };
        strokes << QVector<QPointF>{ QPointF(0.16, 0.12), QPointF(0.17, 0.16) };
    }

    QPainterPath path;
    for (const QVector<QPointF> &stroke : strokes) {
        if (stroke.size() < 2)
            continue;
        path.moveTo(unitToScene(stroke.first().x(), stroke.first().y()));
        for (int i = 1; i < stroke.size(); ++i)
            path.lineTo(unitToScene(stroke.at(i).x(), stroke.at(i).y()));
    }

    p->save();
    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(Theme::withAlpha(line, 44), s * 0.085, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->drawPath(path);
    p->setPen(QPen(line, s * 0.044, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->drawPath(path);
    p->restore();
}

void NightBackdrop::paintYard(QPainter *p, const QRectF &rect, const QColor &line)
{
    p->save();
    p->setBrush(Qt::NoBrush);
    const QPen pen(line, qMax(1.1, rect.height() * 0.018), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p->setPen(pen);

    const qreal baseY = rect.bottom();
    const qreal hx = rect.left() + rect.width() * 0.34;
    const qreal hw = rect.width() * 0.44;
    const qreal hh = rect.height() * 0.44;

    // 屋身 + 坡屋顶
    p->drawPolygon(QVector<QPointF>{
        QPointF(hx - hw / 2.0, baseY), QPointF(hx - hw / 2.0, baseY - hh * 0.52),
        QPointF(hx + hw / 2.0, baseY - hh * 0.52), QPointF(hx + hw / 2.0, baseY)
    });
    p->drawPolygon(QVector<QPointF>{
        QPointF(hx - hw * 0.62, baseY - hh * 0.52), QPointF(hx, baseY - hh * 0.98),
        QPointF(hx + hw * 0.62, baseY - hh * 0.52)
    });
    // 亮着的窗
    p->setBrush(Theme::withAlpha(Theme::windowLit(), 190));
    p->drawRect(QRectF(hx - hw * 0.22, baseY - hh * 0.42, hw * 0.28, hh * 0.22));
    p->setBrush(Qt::NoBrush);
    // 炊烟
    QPainterPath smoke;
    smoke.moveTo(hx + hw * 0.38, baseY - hh * 0.92);
    smoke.cubicTo(hx + hw * 0.52, baseY - hh * 1.16, hx + hw * 0.28, baseY - hh * 1.28,
                  hx + hw * 0.44, baseY - hh * 1.5);
    p->drawPath(smoke);
    // 篱笆
    for (int i = 0; i < 7; ++i) {
        const qreal x = rect.left() + 6.0 + i * (rect.width() - 12.0) / 6.0;
        p->drawLine(QPointF(x, baseY), QPointF(x, baseY - rect.height() * 0.18));
    }
    p->drawLine(QPointF(rect.left() + 6.0, baseY - rect.height() * 0.12),
                QPointF(rect.right() - 6.0, baseY - rect.height() * 0.12));

    p->restore();

    // 院里的树和人
    paintTree(p, QPointF(rect.right() - rect.width() * 0.18, baseY), rect.height() * 0.62,
              Theme::withAlpha(line, 190));
    paintFigure(p, QPointF(hx + hw * 0.62, baseY), rect.height() * 0.30, Theme::withAlpha(line, 200));
}

void NightBackdrop::paintFiveStars(QPainter *p, const QRectF &rect, const QColor &line)
{
    p->save();
    p->setPen(QPen(Theme::withAlpha(line, 60), 7.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->setBrush(line);

    const QPointF big(rect.left() + rect.width() * 0.22, rect.bottom());
    const qreal br = rect.height() * 0.42;
    const QPolygonF starBig = Theme::starPolygon(big, br, br * 0.42);
    p->drawPolygon(starBig);

    const QPointF smalls[] = {
        QPointF(rect.left() + rect.width() * 0.52, rect.top() + rect.height() * 0.30),
        QPointF(rect.left() + rect.width() * 0.64, rect.bottom() - rect.height() * 0.10),
        QPointF(rect.left() + rect.width() * 0.74, rect.bottom() - rect.height() * 0.02),
        QPointF(rect.left() + rect.width() * 0.52, rect.bottom() - rect.height() * 0.34)
    };
    const qreal sr = rect.height() * 0.15;
    for (const QPointF &c : smalls)
        p->drawPolygon(Theme::starPolygon(c, sr, sr * 0.42));
    p->restore();
}

void NightBackdrop::paintCampusTower(QPainter *p, const QRectF &rect, const QColor &line)
{
    p->save();
    p->setBrush(Qt::NoBrush);
    const qreal cx = rect.center().x();
    const qreal bottom = rect.bottom();
    const qreal bodyH = rect.height() * 0.62;
    const qreal w = rect.width() * 0.52;

    // 塔身
    p->drawPolygon(QVector<QPointF>{
        QPointF(cx - w / 2.0, bottom), QPointF(cx - w / 2.0, bottom - bodyH),
        QPointF(cx + w / 2.0, bottom - bodyH), QPointF(cx + w / 2.0, bottom)
    });
    // 顶部机房 + 尖顶
    const qreal topY = bottom - bodyH;
    p->drawPolygon(QVector<QPointF>{
        QPointF(cx - w * 0.62, topY), QPointF(cx - w * 0.62, topY - rect.height() * 0.10),
        QPointF(cx + w * 0.62, topY - rect.height() * 0.10), QPointF(cx + w * 0.62, topY)
    });
    p->drawPolygon(QVector<QPointF>{
        QPointF(cx - w * 0.46, topY - rect.height() * 0.10), QPointF(cx, topY - rect.height() * 0.24),
        QPointF(cx + w * 0.46, topY - rect.height() * 0.10)
    });
    // 旗杆
    p->drawLine(QPointF(cx, topY - rect.height() * 0.24), QPointF(cx, topY - rect.height() * 0.34));
    p->setBrush(Theme::withAlpha(Theme::chinaRed(), 220));
    p->drawPolygon(QVector<QPointF>{
        QPointF(cx, topY - rect.height() * 0.34), QPointF(cx + w * 0.42, topY - rect.height() * 0.31),
        QPointF(cx, topY - rect.height() * 0.28)
    });
    p->setBrush(Qt::NoBrush);

    // 时钟
    const QPointF clock(cx, bottom - bodyH * 0.66);
    const qreal cr = w * 0.30;
    p->drawEllipse(clock, cr, cr);
    p->drawLine(clock, QPointF(clock.x() + cr * 0.42, clock.y() - cr * 0.36));
    p->drawLine(clock, QPointF(clock.x(), clock.y() - cr * 0.66));

    // 窗
    for (int i = 0; i < 3; ++i) {
        const qreal y = bottom - bodyH * 0.30 - i * rect.height() * 0.09;
        p->setBrush(Theme::withAlpha(Theme::windowLit(), 120));
        p->drawRect(QRectF(cx - w * 0.16, y - rect.height() * 0.06, w * 0.32, rect.height() * 0.06));
        p->setBrush(Qt::NoBrush);
    }
    p->restore();
}

void NightBackdrop::paintBike(QPainter *p, const QPointF &base, qreal scale, const QColor &line)
{
    p->save();
    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(line, 1.8 * scale, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const qreal r = 17.0 * scale;
    const QPointF wheelL(base.x() - 30.0 * scale, base.y() - r);
    const QPointF wheelR(base.x() + 30.0 * scale, base.y() - r);
    p->drawEllipse(wheelL, r, r);
    p->drawEllipse(wheelR, r, r);

    const QPointF seat(base.x() - 8.0 * scale, base.y() - 44.0 * scale);
    const QPointF handle(base.x() + 22.0 * scale, base.y() - 48.0 * scale);
    p->drawLine(wheelL + QPointF(0, 0), seat);
    p->drawLine(seat, QPointF(base.x() + 10.0 * scale, base.y() - 24.0 * scale));
    p->drawLine(QPointF(base.x() + 10.0 * scale, base.y() - 24.0 * scale), wheelR);
    p->drawLine(wheelR, handle);
    p->drawLine(handle, QPointF(base.x() + 6.0 * scale, base.y() - 30.0 * scale));
    p->drawLine(QPointF(base.x() - 8.0 * scale, base.y() - 44.0 * scale),
                QPointF(base.x() + 8.0 * scale, base.y() - 44.0 * scale));
    p->restore();
}

void NightBackdrop::paintFigure(QPainter *p, const QPointF &feet, qreal height, const QColor &fill)
{
    p->save();
    p->setPen(Qt::NoPen);
    p->setBrush(fill);
    const qreal headR = height * 0.11;
    p->drawEllipse(QPointF(feet.x(), feet.y() - height + headR), headR, headR * 1.12);
    const qreal bodyTop = feet.y() - height + headR * 2.2;
    p->drawPolygon(QVector<QPointF>{
        QPointF(feet.x() - height * 0.10, bodyTop), QPointF(feet.x() + height * 0.10, bodyTop),
        QPointF(feet.x() + height * 0.07, feet.y() - height * 0.30),
        QPointF(feet.x() - height * 0.07, feet.y() - height * 0.30)
    });
    p->drawRect(QRectF(feet.x() - height * 0.055, feet.y() - height * 0.32, height * 0.045, height * 0.32));
    p->drawRect(QRectF(feet.x() + height * 0.010, feet.y() - height * 0.32, height * 0.045, height * 0.32));
    p->restore();
}

void NightBackdrop::paintTree(QPainter *p, const QPointF &base, qreal height, const QColor &fill)
{
    p->save();
    p->setPen(Qt::NoPen);
    p->setBrush(fill);
    p->drawRect(QRectF(base.x() - height * 0.030, base.y() - height * 0.56, height * 0.060, height * 0.56));
    p->drawEllipse(QPointF(base.x(), base.y() - height * 0.72), height * 0.33, height * 0.27);
    p->drawEllipse(QPointF(base.x() - height * 0.22, base.y() - height * 0.62), height * 0.23, height * 0.19);
    p->drawEllipse(QPointF(base.x() + height * 0.22, base.y() - height * 0.62), height * 0.23, height * 0.19);
    p->restore();
}

/* ========================================================================== */
/*  NightScene                                                                */
/* ========================================================================== */
NightScene::NightScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setSceneRect(0.0, 0.0, qreal(Theme::kSceneW), qreal(Theme::kSceneH));
    m_backdrop = new NightBackdrop();
    addItem(m_backdrop);

    connect(&m_timer, &QTimer::timeout, this, &NightScene::onTick);
    connect(m_backdrop, &NightBackdrop::requestGame, this, &NightScene::requestGame);
}

NightScene::~NightScene() = default;

void NightScene::startClock()
{
    m_clock.restart();
    m_timer.start(16);
}

void NightScene::stopClock()
{
    m_timer.stop();
}

void NightScene::onTick()
{
    const qint64 elapsed = m_clock.restart();
    const qreal dt = qreal(elapsed) / 1000.0;
    if (m_backdrop)
        m_backdrop->tick(qMin(dt, 0.05));
}

void NightScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_backdrop)
        m_backdrop->beginDrag(event->scenePos());
    QGraphicsScene::mousePressEvent(event);
}

void NightScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_backdrop) {
        m_backdrop->setPointer(event->scenePos());
        m_backdrop->moveDrag(event->scenePos());
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void NightScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_backdrop)
        m_backdrop->endDrag();
    QGraphicsScene::mouseReleaseEvent(event);
}

void NightScene::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        emit requestStart();
    QGraphicsScene::keyPressEvent(event);
}

/* ========================================================================== */
/*  NightSceneView                                                            */
/* ========================================================================== */
NightSceneView::NightSceneView(NightScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setBackgroundBrush(QBrush(Theme::nightTop()));
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    updateTransform();
}

void NightSceneView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    updateTransform();
}

void NightSceneView::updateTransform()
{
    if (!scene())
        return;
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}
