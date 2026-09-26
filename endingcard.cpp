/*
 * endingcard.cpp
 * 结尾页实现：校训石 + 祝福卡 + 六个词 + 重玩/返回。
 */

#include "endingcard.h"

#include <QFontMetrics>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QtGlobal>

#include <cmath>

namespace {

struct Star
{
    QPointF pos;
    qreal size;
    qreal phase;
};

QVector<Star> makeStars()
{
    QVector<Star> stars;
    quint32 seed = 88991u;
    auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };
    for (int i = 0; i < 120; ++i) {
        Star s;
        s.pos = QPointF(qreal(next() % 128000u) / 100.0, qreal(next() % 62000u) / 100.0);
        s.size = 0.8 + qreal(next() % 150u) / 100.0;
        s.phase = qreal(next() % 628u) / 100.0;
        stars << s;
    }
    return stars;
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

EndingCard::EndingCard(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // 上升的金色余烬
    quint32 seed = 7331u;
    auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };
    for (int i = 0; i < 46; ++i) {
        Ember e;
        e.pos = QPointF(qreal(next() % 128000u) / 100.0, qreal(next() % 72000u) / 100.0);
        e.vy = 12.0 + qreal(next() % 2200u) / 100.0;
        e.size = 1.4 + qreal(next() % 260u) / 100.0;
        e.phase = qreal(next() % 628u) / 100.0;
        m_embers << e;
    }

    updateTransform();

    // 实拍素材：校训楼夜色底图 + 原图书法
    m_bgImg = Theme::optionalImage(QStringLiteral("ending_card.jpg"));
    m_mottoImg = Theme::optionalImage(QStringLiteral("ending_motto.png"));
    m_moonImg = Theme::optionalImage(QStringLiteral("moon.png"));

    connect(&m_timer, &QTimer::timeout, this, &EndingCard::tick);
    m_timer.start(16);
    m_clock.start();
}

void EndingCard::setResult(int score, const QStringList &words)
{
    m_score = score;
    m_words = words;
    m_time = 0.0;
    update();
}

void EndingCard::updateTransform()
{
    const qreal sx = width() > 0 ? width() / qreal(Theme::kSceneW) : 1.0;
    const qreal sy = height() > 0 ? height() / qreal(Theme::kSceneH) : 1.0;
    m_scale = qMin(sx, sy);
    if (m_scale <= 0.0)
        m_scale = 1.0;
    m_offsetX = (width() - qreal(Theme::kSceneW) * m_scale) / 2.0;
    m_offsetY = (height() - qreal(Theme::kSceneH) * m_scale) / 2.0;
}

QPointF EndingCard::toLogical(const QPointF &widgetPos) const
{
    return QPointF((widgetPos.x() - m_offsetX) / m_scale, (widgetPos.y() - m_offsetY) / m_scale);
}

void EndingCard::tick()
{
    const qint64 elapsed = m_clock.restart();
    const qreal dt = qMin(qreal(elapsed) / 1000.0, 0.05);
    m_time += dt;

    for (Ember &e : m_embers) {
        e.pos.setY(e.pos.y() - e.vy * dt);
        if (e.pos.y() < -10.0)
            e.pos.setY(qreal(Theme::kSceneH) + 10.0);
    }
    update();
}

void EndingCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    updateTransform();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.translate(m_offsetX, m_offsetY);
    p.scale(m_scale, m_scale);

    drawBackground(&p);
    drawMotto(&p);
    drawGreetingCard(&p);
    drawWords(&p);
    drawCalligraphy(&p);
    drawButtons(&p);
}

void EndingCard::drawBackground(QPainter *p) const
{
    const QRectF full(0.0, 0.0, qreal(Theme::kSceneW), qreal(Theme::kSceneH));

    if (!m_bgImg.isNull()) {
        // 校训楼夜色实拍：等比裁剪铺满 + 夜色压暗，保证前景文字全部读得清
        Theme::drawCover(p, full, m_bgImg, 128, 172);
        QLinearGradient mid(0.0, 120.0, 0.0, 620.0);
        mid.setColorAt(0.0, QColor(4, 7, 18, 0));
        mid.setColorAt(0.5, QColor(4, 7, 18, 68));
        mid.setColorAt(1.0, QColor(4, 7, 18, 26));
        p->setPen(Qt::NoPen);
        p->fillRect(QRectF(0.0, 120.0, qreal(Theme::kSceneW), 500.0), mid);
    } else {
        QLinearGradient sky(0.0, 0.0, 0.0, qreal(Theme::kSceneH));
        sky.setColorAt(0.0, Theme::nightTop());
        sky.setColorAt(0.55, QColor(16, 22, 46));
        sky.setColorAt(1.0, Theme::mixColor(Theme::nightBottom(), Theme::chinaRed(), 0.18));
        p->setPen(Qt::NoPen);
        p->setBrush(sky);
        p->drawRect(full);
    }

    // 星（只撒在屋檐以上的夜空，不往楼体上撒）
    const QVector<Star> stars = makeStars();
    p->setPen(Qt::NoPen);
    for (const Star &s : stars) {
        if (!m_bgImg.isNull() && s.pos.y() > 250.0)
            continue;
        const qreal tw = 0.35 + 0.65 * std::sin(m_time * 1.3 + s.phase);
        p->setBrush(Theme::withAlpha(Theme::warmWhite(), int(150.0 * tw)));
        p->drawEllipse(s.pos, s.size, s.size);
    }

    // 月：与主场景、启动页共用同一张 images/moon.png
    const QPointF moon(1096.0, 104.0);
    const qreal moonR = 46.0;
    QRadialGradient halo(moon, moonR * 3.4);
    halo.setColorAt(0.0, Theme::withAlpha(Theme::lightGold(), 118));
    halo.setColorAt(0.4, Theme::withAlpha(Theme::gold(), 32));
    halo.setColorAt(1.0, Theme::withAlpha(Theme::gold(), 0));
    p->setPen(Qt::NoPen);
    p->setBrush(halo);
    p->drawEllipse(moon, moonR * 3.4, moonR * 3.4);
    if (!m_moonImg.isNull()) {
        p->setRenderHint(QPainter::SmoothPixmapTransform, true);
        const qreal box = moonR * 1.9;
        p->drawPixmap(QRectF(moon.x() - box, moon.y() - box, box * 2.0, box * 2.0).toRect(),
                      m_moonImg);
    } else {
        p->setBrush(Theme::cream());
        p->drawEllipse(moon, moonR, moonR);
    }

    // 上升金粉
    for (const Ember &e : m_embers) {
        const qreal tw = 0.5 + 0.5 * std::sin(m_time * 2.0 + e.phase);
        p->setBrush(Theme::withAlpha(Theme::lightGold(), int(130.0 * tw)));
        p->drawEllipse(e.pos, e.size, e.size);
    }

    // 顶部学校署名
    p->setFont(Theme::font(12));
    p->setPen(Theme::withAlpha(Theme::cream(), 150));
    p->drawText(QRectF(0.0, 22.0, qreal(Theme::kSceneW), 18.0), Qt::AlignCenter,
                QStringLiteral("%1 ｜ %2").arg(Theme::schoolName(), Theme::activityTheme()));
}

void EndingCard::drawMotto(QPainter *p) const
{
    const QRectF stone(310.0, 70.0, 660.0, 124.0);
    const qreal appear = Theme::easeOutCubic(Theme::clamp01(m_time / 0.9));
    p->save();
    p->setOpacity(appear);

    // 石身
    QLinearGradient rock(stone.topLeft(), stone.bottomLeft());
    rock.setColorAt(0.0, QColor(96, 94, 88, 232));
    rock.setColorAt(0.45, QColor(64, 63, 60, 236));
    rock.setColorAt(1.0, QColor(38, 37, 36, 240));
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 96), 2.0));
    p->setBrush(rock);
    p->drawRoundedRect(stone, 18.0, 18.0);

    // 顶沿一道极淡的高光，让石面有一点厚度感。
    // 注意别在这里画"石纹"横线：深色石面上那会变成一排很扎眼的扫描线。
    p->setPen(QPen(Theme::withAlpha(Theme::warmWhite(), 34), 1.4));
    p->drawLine(QPointF(stone.left() + 26.0, stone.top() + 6.0),
                QPointF(stone.right() - 26.0, stone.top() + 6.0));

    // 校训
    const qreal pulse = 0.82 + 0.18 * std::sin(m_time * 2.1);
    p->setFont(Theme::font(40, true));
    Theme::drawGlowText(p, QPointF(stone.center().x(), stone.center().y() - 8.0),
                        Theme::motto(),
                        Theme::cream(), Theme::withAlpha(Theme::gold(), int(92.0 * pulse)), 6.0);

    p->setFont(Theme::font(11));
    p->setPen(Theme::withAlpha(Theme::cream(), 145));
    p->drawText(QRectF(stone.left(), stone.bottom() - 26.0, stone.width(), 18.0), Qt::AlignCenter,
                QStringLiteral("MOTTO · 河南师范大学校训"));
    p->restore();
}

void EndingCard::drawGreetingCard(QPainter *p) const
{
    const QRectF card(340.0, 226.0, 600.0, 182.0);
    const qreal appear = Theme::easeOutCubic(Theme::clamp01((m_time - 0.5) / 0.9));
    p->save();
    p->setOpacity(appear);

    // 卡面：中国红渐变，直接压在实拍底图上，靠描金边与红雾拉开层次
    QLinearGradient face(card.topLeft(), card.bottomLeft());
    face.setColorAt(0.0, QColor(178, 28, 38, 236));
    face.setColorAt(0.55, QColor(148, 17, 29, 240));
    face.setColorAt(1.0, QColor(104, 10, 21, 244));
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 200), 2.2));
    p->setBrush(face);
    p->drawRoundedRect(card, 16.0, 16.0);

    // 内描金线
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 120), 1.2));
    p->setBrush(Qt::NoBrush);
    p->drawRoundedRect(card.adjusted(12.0, 12.0, -12.0, -12.0), 10.0, 10.0);

    // 主题句
    p->setFont(Theme::font(40, true));
    Theme::drawGlowText(p, QPointF(card.center().x(), card.top() + 70.0),
                        QStringLiteral("以我代码，贺你华诞"),
                        Theme::lightGold(), Theme::withAlpha(Theme::gold(), 92), 6.0);

    p->setFont(Theme::font(14));
    p->setPen(Theme::withAlpha(Theme::cream(), 214));
    p->drawText(QRectF(card.left(), card.top() + 98.0, card.width(), 22.0), Qt::AlignCenter,
                QStringLiteral("留校不是缺席团圆，而是把小家的思念写进大家的代码里"));

    p->setFont(Theme::font(12));
    p->setPen(Theme::withAlpha(Theme::cream(), 168));
    p->drawText(QRectF(card.left(), card.bottom() - 34.0, card.width(), 20.0), Qt::AlignCenter,
                QStringLiteral("2026 · 国庆七十七华诞 · 中秋月圆 · 安阳 · 软件学院"));

    // 印章：深红底 + 双圈金边 + 金字竖排，比一整块实心金更像一枚钤印
    const QPointF seal(card.right() - 48.0, card.bottom() - 44.0);
    const QRectF sealBox(seal.x() - 23.0, seal.y() - 23.0, 46.0, 46.0);
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(112, 12, 24, 236));
    p->drawRoundedRect(sealBox, 5.0, 5.0);
    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 232), 2.0));
    p->drawRoundedRect(sealBox, 5.0, 5.0);
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 120), 1.0));
    p->drawRoundedRect(sealBox.adjusted(4.5, 4.5, -4.5, -4.5), 3.0, 3.0);
    p->setPen(Theme::lightGold());
    p->setFont(Theme::font(15, true));
    p->drawText(QRectF(sealBox.left(), sealBox.top() + 3.0, sealBox.width(), 22.0),
                Qt::AlignCenter, QStringLiteral("团"));
    p->drawText(QRectF(sealBox.left(), sealBox.top() + 21.0, sealBox.width(), 22.0),
                Qt::AlignCenter, QStringLiteral("圆"));

    p->restore();
}

void EndingCard::drawWords(QPainter *p) const
{
    if (m_words.isEmpty())
        return;

    const qreal appear = Theme::easeOutCubic(Theme::clamp01((m_time - 0.9) / 0.8));
    p->save();
    p->setOpacity(appear);

    p->setFont(Theme::font(18, true));
    const qreal chipW = 86.0;
    const qreal gap = 13.0;
    const qreal totalW = chipW * m_words.size() + gap * qreal(m_words.size() - 1);
    qreal x = (qreal(Theme::kSceneW) - totalW) / 2.0;

    for (int i = 0; i < m_words.size(); ++i) {
        const qreal lift = 4.0 * std::sin(m_time * 2.0 + i * 0.7);
        const QRectF chip(x, 424.0 + lift, chipW, 42.0);
        p->setPen(QPen(Theme::withAlpha(Theme::gold(), 180), 1.4));
        p->setBrush(Theme::withAlpha(QColor(9, 13, 26), 214));
        p->drawRoundedRect(chip, 10.0, 10.0);
        p->setPen(Theme::lightGold());
        p->drawText(chip, Qt::AlignCenter, m_words.at(i));
        x += chipW + gap;
    }

    p->setFont(Theme::font(14));
    p->setPen(Theme::withAlpha(Theme::cream(), 205));
    p->drawText(QRectF(0.0, 474.0, qreal(Theme::kSceneW), 20.0), Qt::AlignCenter,
                QStringLiteral("本局得分 %1 · 六个词已全部点亮").arg(m_score));
    p->restore();
}

/* 右下角缀原图书法「楼倚暮霞 德韵扬辉」：把用户提供的照片元素也用进画面 */
void EndingCard::drawCalligraphy(QPainter *p) const
{
    if (m_mottoImg.isNull())
        return;

    const qreal appear = Theme::easeOutCubic(Theme::clamp01((m_time - 1.2) / 1.0));
    const qreal w = 252.0;
    const qreal h = w * qreal(m_mottoImg.height()) / qreal(m_mottoImg.width());
    const QRectF box(qreal(Theme::kSceneW) - w - 26.0,
                     qreal(Theme::kSceneH) - h - 22.0, w, h);

    p->save();
    p->setOpacity(appear * 0.82);
    p->drawPixmap(box.toRect(), m_mottoImg);
    p->restore();
}

void EndingCard::drawButtons(QPainter *p) const
{
    Theme::drawButton(p, replayButtonRect(), QStringLiteral("再玩一次"), m_hoverReplay, m_pressReplay);
    Theme::drawButton(p, backButtonRect(), QStringLiteral("回到主场景"), m_hoverBack, m_pressBack, false);

    p->setFont(Theme::font(12));
    p->setPen(Theme::withAlpha(Theme::cream(), 150));
    p->drawText(QRectF(0.0, 574.0, qreal(Theme::kSceneW), 20.0), Qt::AlignCenter,
                QStringLiteral("Esc 回到主场景 · R 再接一局"));
}

void EndingCard::mouseMoveEvent(QMouseEvent *event)
{
    m_pointer = toLogical(QPointF(mousePoint(event)));
    m_hoverReplay = replayButtonRect().contains(m_pointer);
    m_hoverBack = backButtonRect().contains(m_pointer);
    update();
    QWidget::mouseMoveEvent(event);
}

void EndingCard::mousePressEvent(QMouseEvent *event)
{
    m_pointer = toLogical(QPointF(mousePoint(event)));
    m_pressReplay = replayButtonRect().contains(m_pointer);
    m_pressBack = backButtonRect().contains(m_pointer);
    update();
    QWidget::mousePressEvent(event);
}

void EndingCard::mouseReleaseEvent(QMouseEvent *event)
{
    m_pointer = toLogical(QPointF(mousePoint(event)));
    if (m_pressReplay && replayButtonRect().contains(m_pointer))
        emit replayRequested();
    if (m_pressBack && backButtonRect().contains(m_pointer))
        emit backRequested();
    m_pressReplay = false;
    m_pressBack = false;
    update();
    QWidget::mouseReleaseEvent(event);
}

void EndingCard::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        emit backRequested();
    if (event->key() == Qt::Key_R)
        emit replayRequested();
    QWidget::keyPressEvent(event);
}

void EndingCard::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateTransform();
}
