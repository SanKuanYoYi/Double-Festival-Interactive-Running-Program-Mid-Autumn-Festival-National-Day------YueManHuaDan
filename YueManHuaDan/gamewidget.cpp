/*
 * gamewidget.cpp
 * 接月饼小游戏实现。
 *
 * 窗户 layout：64 x 20 = 1280 扇窗户，铺在一栋实验楼立面上。
 * 玩法：鼠标横向移动托盘接住下落的月饼；每接住一个点亮一扇窗户。
 * 集齐六个词后，已点亮 + 快速点亮的窗户拼出「77」，随后演变为中国地图形状。
 */

#include "gamewidget.h"

#include "soundfx.h"

#include <QFontMetrics>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QPainter>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QtGlobal>

#include <algorithm>

#include <cmath>

#include <cmath>

namespace {

constexpr int kCols = 64;
constexpr int kRows = 20;
const QRectF kGridArea(150.0, 70.0, 980.0, 486.0);
const QRectF kFacade(130.0, 56.0, 1020.0, 516.0);
constexpr qreal kBasketY = 646.0;
constexpr qreal kBasketW = 180.0;
constexpr qreal kBasketH = 26.0;

const char *kSevenBitmap[] = {
    "1111111",
    "0000001",
    "0000011",
    "0000110",
    "0001100",
    "0011000",
    "0110000",
    "1100000",
    "1100000"
};

quint32 gSeed = 20261001u;
quint32 nextRand()
{
    gSeed = gSeed * 1664525u + 1013904223u;
    return gSeed;
}
qreal randUnit() { return qreal(nextRand() % 100000u) / 100000.0; }
qreal randRange(qreal a, qreal b) { return a + (b - a) * randUnit(); }

struct Star
{
    QPointF pos;
    qreal size;
    qreal phase;
};

QVector<Star> makeStars()
{
    quint32 seed = 31415926u;
    auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };
    QVector<Star> stars;
    for (int i = 0; i < 90; ++i) {
        Star s;
        s.pos = QPointF(qreal(next() % 128000u) / 100.0, qreal(next() % 46000u) / 100.0);
        s.size = 0.8 + qreal(next() % 140u) / 100.0;
        s.phase = qreal(next() % 628u) / 100.0;
        stars << s;
    }
    return stars;
}

/*
 * 中国轮廓（写意示意版）：点为经纬度近似值，按官方口径绘制，
 * 大陆、海南岛、台湾岛均属中国领土。
 * 注：这是用于窗户点阵的艺术化示意，不作为地理参考底图。
 */
QVector<QPointF> mainlandOutline()
{
    return QVector<QPointF>{
        QPointF(120.5, 53.3), QPointF(125.5, 53.0), QPointF(127.0, 50.5),
        QPointF(130.7, 48.9), QPointF(134.7, 48.4), QPointF(133.0, 45.2),
        QPointF(131.3, 44.0), QPointF(130.6, 42.7), QPointF(128.1, 41.4),
        QPointF(126.0, 40.9), QPointF(124.4, 40.0), QPointF(122.1, 39.4),
        QPointF(121.2, 38.7), QPointF(122.7, 37.4), QPointF(120.3, 37.8),
        QPointF(119.0, 35.0), QPointF(120.8, 34.0), QPointF(121.9, 31.9),
        QPointF(121.5, 30.8), QPointF(120.2, 29.0), QPointF(120.6, 27.4),
        QPointF(119.3, 26.4), QPointF(117.5, 24.5), QPointF(116.0, 23.0),
        QPointF(114.3, 22.6), QPointF(113.0, 21.7), QPointF(111.0, 21.5),
        QPointF(109.5, 21.5), QPointF(108.1, 21.6), QPointF(106.7, 22.1),
        QPointF(105.5, 23.2), QPointF(103.5, 22.7), QPointF(101.8, 21.2),
        QPointF(100.1, 21.5), QPointF(99.2, 22.5), QPointF(97.5, 23.9),
        QPointF(98.3, 25.6), QPointF(98.7, 27.5), QPointF(97.0, 28.3),
        QPointF(96.0, 29.0), QPointF(93.5, 28.3), QPointF(91.6, 27.7),
        QPointF(89.0, 27.5), QPointF(86.5, 28.2), QPointF(83.0, 29.5),
        QPointF(81.0, 30.3), QPointF(79.0, 32.0), QPointF(78.2, 34.5),
        QPointF(76.5, 35.8), QPointF(74.9, 37.0), QPointF(73.6, 38.6),
        QPointF(75.0, 40.4), QPointF(76.5, 41.0), QPointF(79.5, 42.0),
        QPointF(80.3, 43.2), QPointF(82.6, 45.1), QPointF(85.0, 47.0),
        QPointF(87.3, 48.5), QPointF(90.0, 47.8), QPointF(91.0, 45.2),
        QPointF(95.4, 44.0), QPointF(96.4, 42.7), QPointF(99.5, 42.6),
        QPointF(102.5, 42.2), QPointF(105.5, 41.5), QPointF(108.5, 42.4),
        QPointF(111.0, 43.5), QPointF(114.0, 45.2), QPointF(116.5, 46.4),
        QPointF(119.0, 47.7), QPointF(119.7, 49.5), QPointF(117.5, 49.8),
        QPointF(116.5, 50.5), QPointF(119.5, 51.5), QPointF(121.0, 53.0)
    };
}

QVector<QPointF> islandHainan()
{
    return QVector<QPointF>{
        QPointF(109.0, 20.1), QPointF(110.1, 20.1), QPointF(110.6, 19.6),
        QPointF(111.0, 19.2), QPointF(110.5, 18.6), QPointF(109.5, 18.4),
        QPointF(108.7, 19.0), QPointF(108.6, 19.6)
    };
}

QVector<QPointF> islandTaiwan()
{
    return QVector<QPointF>{
        QPointF(121.0, 25.3), QPointF(121.9, 25.1), QPointF(121.9, 24.0),
        QPointF(120.9, 22.6), QPointF(120.3, 22.5), QPointF(120.1, 23.3),
        QPointF(120.5, 24.5)
    };
}

QPoint mousePoint(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position().toPoint();
#else
    return event->pos();
#endif
}

} // namespace

/* ========================================================================== */
GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setFocusPolicy(Qt::StrongFocus);

    m_words = QStringList{ QStringLiteral("团圆"), QStringLiteral("昌盛"),
                           QStringLiteral("华诞"), QStringLiteral("厚德"),
                           QStringLiteral("博学"), QStringLiteral("至善") };
    m_wordCount = QVector<int>(m_words.size(), 0);

    m_cakeImg.load(QStringLiteral(":/images/mooncake.png"));
    m_sparkImg.load(QStringLiteral(":/images/spark.png"));

    const int cellTotal = kCols * kRows;
    m_cellBright = QVector<qreal>(cellTotal, 0.0);
    m_cellTarget = QVector<qreal>(cellTotal, 0.0);
    m_cellPop = QVector<qreal>(cellTotal, 0.0);

    buildPatterns();
    updateTransform();

    connect(&m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer.start(16);
    m_clock.start();
}

/* ---------------- 布局换算 ---------------- */
void GameWidget::updateTransform()
{
    const qreal sx = width() > 0 ? width() / qreal(Theme::kSceneW) : 1.0;
    const qreal sy = height() > 0 ? height() / qreal(Theme::kSceneH) : 1.0;
    m_scale = qMin(sx, sy);
    if (m_scale <= 0.0)
        m_scale = 1.0;
    m_offsetX = (width() - qreal(Theme::kSceneW) * m_scale) / 2.0;
    m_offsetY = (height() - qreal(Theme::kSceneH) * m_scale) / 2.0;
}

QPointF GameWidget::toLogical(const QPointF &widgetPos) const
{
    return QPointF((widgetPos.x() - m_offsetX) / m_scale, (widgetPos.y() - m_offsetY) / m_scale);
}

QRectF GameWidget::basketRect() const
{
    return QRectF(m_basketX - kBasketW / 2.0, kBasketY, kBasketW, kBasketH);
}

QRectF GameWidget::backButtonRect() const
{
    return QRectF(24.0, 24.0, 118.0, 42.0);
}

QRectF GameWidget::windowRect(int col, int row) const
{
    const qreal cw = kGridArea.width() / kCols;
    const qreal ch = kGridArea.height() / kRows;
    const qreal ww = cw * 0.62;
    const qreal wh = ch * 0.70;
    return QRectF(kGridArea.left() + col * cw + (cw - ww) / 2.0,
                  kGridArea.top() + row * ch + (ch - wh) / 2.0, ww, wh);
}

/* ---------------- 图案：77 与中国地图 ---------------- */
void GameWidget::buildPatterns()
{
    m_sevenCells.clear();

    const int scale = 2;                      // 位图放大倍数
    const int bw = 7 * scale;                 // 单个「7」的宽度（格）
    const int bh = 9 * scale;                 // 高度（格）
    const int gap = 3;
    const int startCol = (kCols - (bw * 2 + gap)) / 2;
    const int startRow = (kRows - bh) / 2;

    for (int d = 0; d < 2; ++d) {
        const int baseCol = startCol + d * (bw + gap);
        for (int r = 0; r < 9; ++r) {
            const char *line = kSevenBitmap[r];
            for (int c = 0; c < 7; ++c) {
                if (line[c] != '1')
                    continue;
                for (int sy = 0; sy < scale; ++sy) {
                    for (int sx = 0; sx < scale; ++sx) {
                        const int col = baseCol + c * scale + sx;
                        const int row = startRow + r * scale + sy;
                        if (col < 0 || col >= kCols || row < 0 || row >= kRows)
                            continue;
                        m_sevenCells << Cell{ col, row };
                    }
                }
            }
        }
    }

    /* 中国地图轮廓：等经纬比例映射到窗户网格 */
    m_mapCells.clear();
    const QVector<QPointF> mainland = mainlandOutline();
    if (mainland.size() < 3)
        return;

    const qreal lonMin = 73.0, lonMax = 135.5;
    const qreal latMin = 17.8, latMax = 53.8;
    const qreal aspect = (lonMax - lonMin) * 0.82 / (latMax - latMin);   // 约 1.43
    const qreal mapH = kGridArea.height() * 0.86;
    const qreal mapW = mapH * aspect;
    const QRectF mapPix(kGridArea.left() + (kGridArea.width() - mapW) / 2.0,
                        kGridArea.top() + (kGridArea.height() - mapH) / 2.0,
                        mapW, mapH);

    const qreal cw = kGridArea.width() / kCols;
    const qreal ch = kGridArea.height() / kRows;

    auto toCell = [&](const QPointF &lonLat, QPointF *out) {
        const qreal x = mapPix.left() + (lonLat.x() - lonMin) / (lonMax - lonMin) * mapW;
        const qreal y = mapPix.top() + (latMax - lonLat.y()) / (latMax - latMin) * mapH;
        *out = QPointF((x - kGridArea.left()) / cw, (y - kGridArea.top()) / ch);
    };

    QVector<QPointF> mainCells;
    for (const QPointF &p : mainland) {
        QPointF c;
        toCell(p, &c);
        mainCells << c;
    }
    QVector<QPointF> hainanCells;
    for (const QPointF &p : islandHainan()) {
        QPointF c;
        toCell(p, &c);
        hainanCells << c;
    }
    QVector<QPointF> taiwanCells;
    for (const QPointF &p : islandTaiwan()) {
        QPointF c;
        toCell(p, &c);
        taiwanCells << c;
    }

    QVector<bool> inside(kCols * kRows, false);
    QVector<bool> small(kCols * kRows, false);
    for (int row = 0; row < kRows; ++row) {
        for (int col = 0; col < kCols; ++col) {
            const QPointF center(col + 0.5, row + 0.5);
            const int idx = row * kCols + col;
            if (Theme::pointInPolygon(center, mainCells))
                inside[idx] = true;
            if (Theme::pointInPolygon(center, hainanCells) || Theme::pointInPolygon(center, taiwanCells))
                small[idx] = true;
        }
    }

    for (int row = 0; row < kRows; ++row) {
        for (int col = 0; col < kCols; ++col) {
            const int idx = row * kCols + col;
            if (small[idx]) {
                m_mapCells << Cell{ col, row };
                continue;
            }
            if (!inside[idx])
                continue;
            const bool top = (row > 0) && !inside[(row - 1) * kCols + col];
            const bool bottom = (row < kRows - 1) && !inside[(row + 1) * kCols + col];
            const bool left = (col > 0) && !inside[row * kCols + col - 1];
            const bool right = (col < kCols - 1) && !inside[row * kCols + col + 1];
            if (top || bottom || left || right)
                m_mapCells << Cell{ col, row };
        }
    }

    // 以地图中心为原点按角度排序，点亮时像一轮日光扫过国土
    QPointF sum(0.0, 0.0);
    for (const Cell &c : m_mapCells)
        sum += QPointF(c.col, c.row);
    const QPointF mid = !m_mapCells.isEmpty() ? sum / qreal(m_mapCells.size()) : QPointF();
    std::sort(m_mapCells.begin(), m_mapCells.end(), [mid](const Cell &a, const Cell &b) {
        const qreal aa = std::atan2(qreal(a.row) - mid.y(), qreal(a.col) - mid.x());
        const qreal ab = std::atan2(qreal(b.row) - mid.y(), qreal(b.col) - mid.x());
        return aa < ab;
    });
}

/* ---------------- 每帧推进 ---------------- */
void GameWidget::tick()
{
    const qint64 elapsed = m_clock.restart();
    const qreal dt = qMin(qreal(elapsed) / 1000.0, 0.05);
    m_time += dt;

    for (int i = m_puffs.size() - 1; i >= 0; --i) {
        m_puffs[i].life -= dt * 1.6;
        m_puffs[i].pos.setY(m_puffs[i].pos.y() - dt * 28.0);
        if (m_puffs[i].life <= 0.0)
            m_puffs.removeAt(i);
    }

    if (m_phase == Playing) {
        m_spawnTimer += dt;
        const qreal interval = qMax(0.46, m_spawnInterval - m_time * 0.006);
        if (m_spawnTimer >= interval) {
            m_spawnTimer = 0.0;
            spawn();
        }
    }

    const QRectF basket = basketRect();
    for (int i = m_cakes.size() - 1; i >= 0; --i) {
        Mooncake &cake = m_cakes[i];
        cake.rect.moveTop(cake.rect.top() + cake.vy * dt);
        cake.rot += cake.vr * dt;

        if (m_phase == Playing && cake.rect.intersects(basket)) {
            const int wi = m_words.indexOf(cake.word);
            if (wi >= 0)
                m_wordCount[wi] = m_wordCount.at(wi) + 1;
            ++m_caught;
            m_score += 10;
            lightNextCell();
            SoundFx::playCatch();
            for (int k = 0; k < 8; ++k) {
                Puff puff;
                puff.pos = cake.rect.center() + QPointF(randRange(-16.0, 16.0), randRange(-10.0, 10.0));
                puff.life = 1.0;
                m_puffs << puff;
            }
            m_cakes.removeAt(i);
            continue;
        }
        if (cake.rect.top() > qreal(Theme::kSceneH) + 24.0) {
            if (m_phase == Playing)
                ++m_missed;
            m_cakes.removeAt(i);
        }
    }

    // 胜利判定：六个词各集齐 need 次
    if (m_phase == Playing) {
        bool complete = true;
        for (int count : m_wordCount) {
            if (count < m_needPerWord)
                complete = false;
        }
        if (m_time > 150.0)
            complete = true;
        if (complete) {
            m_phase = RevealSeven;
            m_phaseTime = 0.0;
            m_revealAcc = 0.0;
        }
    } else if (m_phase == RevealSeven) {
        m_phaseTime += dt;
        const qreal rate = qreal(m_sevenCells.size()) / 1.8;   // 1.8 秒内补完全部
        m_revealAcc += dt * rate;
        while (m_revealAcc >= 1.0 && m_sevenIndex < m_sevenCells.size()) {
            const Cell &c = m_sevenCells.at(m_sevenIndex++);
            const int idx = c.row * kCols + c.col;
            m_cellTarget[idx] = 1.0;
            m_cellPop[idx] = 1.0;
            m_revealAcc -= 1.0;
        }
        if (m_sevenIndex >= m_sevenCells.size() && m_phaseTime > 2.6) {
            m_phase = MorphMap;
            m_phaseTime = 0.0;
            m_revealAcc = 0.0;
            for (const Cell &c : m_sevenCells)
                m_cellTarget[c.row * kCols + c.col] = 0.0;
        }
    } else if (m_phase == MorphMap) {
        m_phaseTime += dt;
        const qreal rate = qreal(m_mapCells.size()) / 2.6;      // 2.6 秒点亮全图
        m_revealAcc += dt * rate;
        while (m_revealAcc >= 1.0 && m_mapIndex < m_mapCells.size()) {
            const Cell &c = m_mapCells.at(m_mapIndex++);
            const int idx = c.row * kCols + c.col;
            m_cellTarget[idx] = 1.0;
            m_cellPop[idx] = 1.0;
            m_revealAcc -= 1.0;
        }
        if (m_mapIndex >= m_mapCells.size() && m_phaseTime > 3.6) {
            m_phase = Finished;
            m_phaseTime = 0.0;
        }
    } else {
        m_phaseTime += dt;
        if (!m_completed && m_phaseTime > 0.9) {
            m_completed = true;
            emit gameFinished(m_score, m_words);
        }
    }

    updateCells(dt);
    update();
}

void GameWidget::updateCells(qreal dt)
{
    const qreal k = qMin(1.0, dt * 7.5);
    for (int i = 0; i < m_cellBright.size(); ++i) {
        m_cellBright[i] += (m_cellTarget[i] - m_cellBright[i]) * k;
        if (m_cellPop[i] > 0.0)
            m_cellPop[i] = qMax(0.0, m_cellPop[i] - dt * 2.6);
    }
}

void GameWidget::lightNextCell()
{
    if (m_sevenIndex >= m_sevenCells.size())
        return;
    const Cell &c = m_sevenCells.at(m_sevenIndex++);
    const int idx = c.row * kCols + c.col;
    m_cellTarget[idx] = 1.0;
    m_cellPop[idx] = 1.0;
}

void GameWidget::spawn()
{
    QStringList pool;
    for (int i = 0; i < m_words.size(); ++i) {
        if (m_wordCount.at(i) < m_needPerWord)
            pool << m_words.at(i);
    }
    if (pool.isEmpty())
        pool = m_words;

    Mooncake cake;
    const qreal side = randRange(58.0, 74.0);
    const qreal x = randRange(kGridArea.left() + 40.0, kGridArea.right() - 40.0 - side);
    cake.rect = QRectF(x, -side, side, side);
    cake.word = pool.at(int(nextRand() % quint32(pool.size())));
    cake.vy = randRange(155.0, 185.0) * (1.0 + m_time / 55.0);
    cake.rot = randRange(-18.0, 18.0);
    cake.vr = randRange(-40.0, 40.0);
    m_cakes << cake;
}

void GameWidget::restart()
{
    m_cakes.clear();
    m_puffs.clear();
    m_phase = Playing;
    m_time = 0.0;
    m_phaseTime = 0.0;
    m_spawnTimer = 0.0;
    m_revealAcc = 0.0;
    m_score = 0;
    m_caught = 0;
    m_missed = 0;
    m_sevenIndex = 0;
    m_mapIndex = 0;
    m_completed = false;
    std::fill(m_wordCount.begin(), m_wordCount.end(), 0);
    std::fill(m_cellTarget.begin(), m_cellTarget.end(), 0.0);
    std::fill(m_cellBright.begin(), m_cellBright.end(), 0.0);
    std::fill(m_cellPop.begin(), m_cellPop.end(), 0.0);
    update();
}

/* ---------------- 输入 ---------------- */
void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_pointer = toLogical(QPointF(mousePoint(event)));
    m_pointerInside = true;
    m_hoverBack = backButtonRect().contains(m_pointer) ? 1 : 0;
    m_basketX = qBound(kGridArea.left() + kBasketW / 2.0, m_pointer.x(),
                       qreal(Theme::kSceneW) - kBasketW / 2.0 - 20.0);
    QWidget::mouseMoveEvent(event);
}

void GameWidget::mousePressEvent(QMouseEvent *event)
{
    m_pointer = toLogical(QPointF(mousePoint(event)));
    if (backButtonRect().contains(m_pointer))
        m_pressingBack = true;
    update();
    QWidget::mousePressEvent(event);
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_pointer = toLogical(QPointF(mousePoint(event)));
    if (m_pressingBack && backButtonRect().contains(m_pointer))
        emit backRequested();
    m_pressingBack = false;
    update();
    QWidget::mouseReleaseEvent(event);
}

void GameWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        emit backRequested();
    if (event->key() == Qt::Key_R)
        restart();
    QWidget::keyPressEvent(event);
}

void GameWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateTransform();
    m_cacheReady = false;
}

/* ---------------- 绘制 ---------------- */
void GameWidget::ensureCache()
{
    if (m_cacheReady && !m_base.isNull())
        return;
    m_base = QPixmap(Theme::kSceneW, Theme::kSceneH);
    m_base.fill(Qt::transparent);

    QPainter p(&m_base);
    p.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient sky(0.0, 0.0, 0.0, qreal(Theme::kSceneH));
    sky.setColorAt(0.0, Theme::nightTop());
    sky.setColorAt(0.55, QColor(14, 20, 42));
    sky.setColorAt(1.0, Theme::nightBottom());
    p.fillRect(QRectF(0.0, 0.0, qreal(Theme::kSceneW), qreal(Theme::kSceneH)), sky);

    // 静态星
    const QVector<Star> stars = makeStars();
    p.setPen(Qt::NoPen);
    for (const Star &s : stars) {
        p.setBrush(Theme::withAlpha(Theme::warmWhite(), 150));
        p.drawEllipse(s.pos, s.size, s.size);
    }

    // 月亮
    const QPointF moon(1108.0, 118.0);
    QRadialGradient halo(moon, 150.0);
    halo.setColorAt(0.0, Theme::withAlpha(Theme::lightGold(), 90));
    halo.setColorAt(1.0, Theme::withAlpha(Theme::lightGold(), 0));
    p.setBrush(halo);
    p.drawEllipse(moon, 150.0, 150.0);
    p.setBrush(Theme::cream());
    p.drawEllipse(moon, 52.0, 52.0);

    // 地面
    QLinearGradient ground(0.0, 572.0, 0.0, qreal(Theme::kSceneH));
    ground.setColorAt(0.0, QColor(18, 24, 44));
    ground.setColorAt(1.0, QColor(8, 11, 22));
    p.setBrush(ground);
    p.drawRect(QRectF(0.0, 572.0, qreal(Theme::kSceneW), qreal(Theme::kSceneH) - 572.0));

    // 楼体
    p.setBrush(Theme::building());
    p.drawRect(kFacade);
    p.setPen(QPen(Theme::withAlpha(Theme::buildingEdge(), 140), 1.2));
    p.drawRect(kFacade);
    p.setPen(QPen(Theme::withAlpha(QColor(30, 38, 62), 220), 1.0));
    const qreal cwv = kGridArea.width() / kCols;
    for (int c = 0; c <= kCols; ++c) {
        const qreal x = kGridArea.left() + c * cwv;
        p.drawLine(QPointF(x, kGridArea.top() - 8.0), QPointF(x, kGridArea.bottom() + 8.0));
    }
    const qreal chv = kGridArea.height() / kRows;
    for (int r = 0; r <= kRows; ++r) {
        const qreal y = kGridArea.top() + r * chv;
        p.drawLine(QPointF(kGridArea.left() - 8.0, y), QPointF(kGridArea.right() + 8.0, y));
    }

    // 未点亮的窗
    p.setPen(Qt::NoPen);
    p.setBrush(Theme::glassDark());
    for (int row = 0; row < kRows; ++row)
        for (int col = 0; col < kCols; ++col)
            p.drawRect(windowRect(col, row));

    // 楼顶标牌
    p.setBrush(QColor(22, 29, 52));
    p.drawRect(QRectF(kFacade.left() + 320.0, kFacade.top() + 6.0, 380.0, 34.0));
    p.setFont(Theme::font(18, true));
    p.setPen(Theme::withAlpha(Theme::gold(), 220));
    p.drawText(QRectF(kFacade.left() + 320.0, kFacade.top() + 6.0, 380.0, 34.0),
               Qt::AlignCenter, QStringLiteral("留校宿舍 · 今夜为你亮灯"));

    m_cacheReady = true;
}

void GameWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    updateTransform();
    ensureCache();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.translate(m_offsetX, m_offsetY);
    p.scale(m_scale, m_scale);

    drawBackground(&p);
    drawWindows(&p);
    drawCakes(&p);
    drawBasket(&p);

    // 接住时的金色星火
    for (const Puff &puff : m_puffs) {
        const qreal alpha = Theme::clamp01(puff.life);
        const qreal size = 10.0 + 12.0 * (1.0 - alpha);
        if (!m_sparkImg.isNull()) {
            p.setOpacity(alpha * 0.75);
            p.drawPixmap(QRectF(puff.pos.x() - size, puff.pos.y() - size, size * 2.0, size * 2.0).toRect(),
                         m_sparkImg);
            p.setOpacity(1.0);
        } else {
            p.setBrush(Theme::withAlpha(Theme::lightGold(), int(150.0 * alpha)));
            p.setPen(Qt::NoPen);
            p.drawEllipse(puff.pos, size, size);
        }
    }

    drawHud(&p);
}

void GameWidget::drawBackground(QPainter *p) const
{
    p->drawPixmap(QRectF(0.0, 0.0, qreal(Theme::kSceneW), qreal(Theme::kSceneH)), m_base,
                  QRectF(0.0, 0.0, qreal(m_base.width()), qreal(m_base.height())));

    // 星闪
    const QVector<Star> stars = makeStars();
    p->setPen(Qt::NoPen);
    for (int i = 0; i < stars.size(); i += 2) {
        const Star &s = stars.at(i);
        const qreal tw = 0.4 + 0.6 * std::sin(m_time * 1.6 + s.phase);
        p->setBrush(Theme::withAlpha(Theme::warmWhite(), int(200.0 * tw)));
        p->drawEllipse(s.pos, s.size, s.size);
    }
}

void GameWidget::drawWindows(QPainter *p) const
{
    p->setPen(Qt::NoPen);

    // 光晕层
    for (int row = 0; row < kRows; ++row) {
        for (int col = 0; col < kCols; ++col) {
            const int idx = row * kCols + col;
            if (m_cellBright.at(idx) < 0.06)
                continue;
            const QRectF r = windowRect(col, row);
            const qreal b = m_cellBright.at(idx);
            p->setBrush(Theme::withAlpha(Theme::windowLit(), int(46.0 * b)));
            p->drawRect(r.adjusted(-3.0, -3.0, 3.0, 3.0));
        }
    }

    // 窗芯层
    for (int row = 0; row < kRows; ++row) {
        for (int col = 0; col < kCols; ++col) {
            const int idx = row * kCols + col;
            const qreal b = m_cellBright.at(idx);
            if (b < 0.02)
                continue;
            const qreal pop = m_cellPop.at(idx);
            QRectF r = windowRect(col, row);
            if (pop > 0.0)
                r = r.adjusted(-pop * 3.0, -pop * 3.0, pop * 3.0, pop * 3.0);
            const QColor core = Theme::mixColor(Theme::windowLit(), Theme::cream(), pop * 0.8);
            p->setBrush(Theme::withAlpha(core, int(250.0 * Theme::clamp01(b))));
            p->drawRect(r);
        }
    }
}

void GameWidget::drawCakes(QPainter *p) const
{
    for (const Mooncake &cake : m_cakes) {
        p->save();
        p->translate(cake.rect.center());
        p->rotate(cake.rot);
        const QRectF local(-cake.rect.width() / 2.0, -cake.rect.height() / 2.0,
                           cake.rect.width(), cake.rect.height());
        if (!m_cakeImg.isNull())
            p->drawPixmap(local.toRect(), m_cakeImg);
        else {
            p->setBrush(QColor(202, 138, 54));
            p->setPen(Qt::NoPen);
            p->drawEllipse(local);
        }
        p->setFont(Theme::font(14, true));
        p->setPen(Theme::deepRed());
        p->drawText(local, Qt::AlignCenter, cake.word);
        p->restore();
    }
}

void GameWidget::drawBasket(QPainter *p) const
{
    const QRectF basket = basketRect();

    // 光带
    QRadialGradient beam(QPointF(m_basketX, basket.top()), kBasketW * 0.9);
    beam.setColorAt(0.0, Theme::withAlpha(Theme::gold(), 60));
    beam.setColorAt(1.0, Theme::withAlpha(Theme::gold(), 0));
    p->setPen(Qt::NoPen);
    p->setBrush(beam);
    p->drawRect(QRectF(basket.center().x() - kBasketW, basket.top() - 150.0,
                       kBasketW * 2.0, 160.0));

    // 托盘（双手）
    p->setBrush(Theme::mixColor(Theme::chinaRed(), Theme::deepRed(), 0.35));
    p->setPen(QPen(Theme::gold(), 1.6));
    p->drawRoundedRect(basket, 13.0, 13.0);
    p->setBrush(Theme::withAlpha(Theme::cream(), 120));
    p->drawRoundedRect(basket.adjusted(8.0, 4.0, -8.0, -10.0), 8.0, 8.0);
}

void GameWidget::drawHud(QPainter *p) const
{
    // 返回按钮
    Theme::drawButton(p, backButtonRect(), QStringLiteral("← 返回"),
                      m_hoverBack == 1, m_pressingBack, false);

    // 六个词
    const qreal chipW = 96.0;
    const qreal chipH = 50.0;
    const qreal gap = 14.0;
    const qreal totalW = chipW * 6.0 + gap * 5.0;
    qreal x = (qreal(Theme::kSceneW) - totalW) / 2.0;
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 140), 1.4));
    for (int i = 0; i < m_words.size(); ++i) {
        const QRectF chip(x, 22.0, chipW, chipH);
        const bool done = m_wordCount.at(i) >= m_needPerWord;
        if (done) {
            QLinearGradient g(chip.topLeft(), chip.bottomLeft());
            g.setColorAt(0.0, QColor(214, 40, 50));
            g.setColorAt(1.0, Theme::deepRed());
            p->setBrush(g);
        } else {
            p->setBrush(Theme::withAlpha(QColor(12, 16, 32), 190));
        }
        p->drawRoundedRect(chip, 10.0, 10.0);

        p->setFont(Theme::font(19, true));
        p->setPen(done ? Theme::lightGold() : Theme::withAlpha(Theme::cream(), 180));
        p->drawText(QRectF(chip.left(), chip.top() + 6.0, chipW, 24.0), Qt::AlignCenter, m_words.at(i));

        p->setFont(Theme::font(12));
        p->setPen(Theme::withAlpha(done ? Theme::cream() : Theme::gold(), 170));
        p->drawText(QRectF(chip.left(), chip.bottom() - 19.0, chipW, 16.0), Qt::AlignCenter,
                    QStringLiteral("%1/%2").arg(qMin(m_wordCount.at(i), m_needPerWord))
                                           .arg(m_needPerWord));
        p->setPen(QPen(Theme::withAlpha(Theme::gold(), 140), 1.4));
        x += chipW + gap;
    }

    // 计分
    p->setFont(Theme::font(16, true));
    p->setPen(Theme::cream());
    p->drawText(QRectF(qreal(Theme::kSceneW) - 320.0, 26.0, 296.0, 24.0), Qt::AlignRight,
                QStringLiteral("得分 %1    接住 %2    漏接 %3").arg(m_score).arg(m_caught).arg(m_missed));

    // 阶段文案
    QString caption;
    if (m_phase == Playing)
        caption = QStringLiteral("移动鼠标接月饼 · 集齐六个词点亮全楼");
    else if (m_phase == RevealSeven)
        caption = QStringLiteral("留校的窗，拼出「77」—— 祖国生日快乐");
    else if (m_phase == MorphMap)
        caption = QStringLiteral("再一次点亮：金黄的轮廓是咱的中国");
    else
        caption = QStringLiteral("以我代码，贺你华诞");

    p->setFont(Theme::font(22, true));
    Theme::drawGlowText(p, QPointF(qreal(Theme::kSceneW) / 2.0, 612.0), caption,
                        Theme::cream(), Theme::gold(), 6.0);

    p->setFont(Theme::font(13));
    p->setPen(Theme::withAlpha(Theme::cream(), 150));
    p->drawText(QRectF(0.0, 690.0, qreal(Theme::kSceneW), 20.0), Qt::AlignCenter,
                QStringLiteral("每接住一个月饼，就点亮一扇留校的窗 · Esc 返回 · R 重来"));
}
