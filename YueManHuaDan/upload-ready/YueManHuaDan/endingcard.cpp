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

    // 可选卡面素材：不放就沿用矢量绘制的红色卡面
    m_cardImg = Theme::optionalImage(QStringLiteral("ending_card.png"));

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
    drawButtons(&p);
}

void EndingCard::drawBackground(QPainter *p) const
{
    QLinearGradient sky(0.0, 0.0, 0.0, qreal(Theme::kSceneH));
    sky.setColorAt(0.0, Theme::nightTop());
    sky.setColorAt(0.55, QColor(16, 22, 46));
    sky.setColorAt(1.0, Theme::mixColor(Theme::nightBottom(), Theme::chinaRed(), 0.18));
    p->setPen(Qt::NoPen);
    p->setBrush(sky);
    p->drawRect(QRectF(0.0, 0.0, qreal(Theme::kSceneW), qreal(Theme::kSceneH)));

    // 星
    const QVector<Star> stars = makeStars();
    for (const Star &s : stars) {
        const qreal tw = 0.35 + 0.65 * std::sin(m_time * 1.3 + s.phase);
        p->setBrush(Theme::withAlpha(Theme::warmWhite(), int(205.0 * tw)));
        p->drawEllipse(s.pos, s.size, s.size);
    }

    // 月
    const QPointF moon(1054.0, 132.0);
    QRadialGradient halo(moon, 210.0);
    halo.setColorAt(0.0, Theme::withAlpha(Theme::lightGold(), 110));
    halo.setColorAt(0.4, Theme::withAlpha(Theme::gold(), 34));
    halo.setColorAt(1.0, Theme::withAlpha(Theme::gold(), 0));
    p->setBrush(halo);
    p->drawEllipse(moon, 210.0, 210.0);
    p->setBrush(Theme::cream());
    p->drawEllipse(moon, 58.0, 58.0);

    // 地面暖光
    QLinearGradient ground(0.0, 600.0, 0.0, qreal(Theme::kSceneH));
    ground.setColorAt(0.0, Theme::withAlpha(Theme::deepRed(), 40));
    ground.setColorAt(1.0, Theme::withAlpha(QColor(8, 11, 22), 230));
    p->setBrush(ground);
    p->drawRect(QRectF(0.0, 600.0, qreal(Theme::kSceneW), qreal(Theme::kSceneH) - 600.0));

    // 上升金粉
    for (const Ember &e : m_embers) {
        const qreal tw = 0.5 + 0.5 * std::sin(m_time * 2.0 + e.phase);
        p->setBrush(Theme::withAlpha(Theme::lightGold(), int(150.0 * tw)));
        p->drawEllipse(e.pos, e.size, e.size);
    }
}

void EndingCard::drawMotto(QPainter *p) const
{
    const QRectF stone(310.0, 152.0, 660.0, 132.0);
    const qreal appear = Theme::easeOutCubic(Theme::clamp01(m_time / 0.9));
    p->save();
    p->setOpacity(appear);

    // 石身
    QLinearGradient rock(stone.topLeft(), stone.bottomLeft());
    rock.setColorAt(0.0, QColor(74, 76, 84));
    rock.setColorAt(0.45, QColor(52, 54, 62));
    rock.setColorAt(1.0, QColor(32, 33, 40));
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 110), 2.0));
    p->setBrush(rock);
    p->drawRoundedRect(stone, 18.0, 18.0);

    // 石纹
    p->setPen(QPen(Theme::withAlpha(QColor(120, 124, 136), 60), 1.0));
    for (int i = 0; i < 5; ++i) {
        const qreal y = stone.top() + 22.0 + i * 24.0;
        p->drawLine(QPointF(stone.left() + 24.0, y), QPointF(stone.right() - 24.0, y + 3.0));
    }

    // 校训
    const qreal pulse = 0.82 + 0.18 * std::sin(m_time * 2.1);
    p->setFont(Theme::font(46, true));
    Theme::drawGlowText(p, QPointF(stone.center().x(), stone.center().y() - 6.0),
                        QStringLiteral("厚德博学，止于至善"),
                        Theme::cream(), Theme::withAlpha(Theme::gold(), int(140.0 * pulse)), 9.0);

    p->setFont(Theme::font(13));
    p->setPen(Theme::withAlpha(Theme::cream(), 150));
    p->drawText(QRectF(stone.left(), stone.bottom() - 30.0, stone.width(), 20.0), Qt::AlignCenter,
                QStringLiteral("MOTTO · 母校校训"));
    p->restore();
}

void EndingCard::drawGreetingCard(QPainter *p) const
{
    const QRectF card(340.0, 322.0, 600.0, 196.0);
    const qreal appear = Theme::easeOutCubic(Theme::clamp01((m_time - 0.5) / 0.9));
    p->save();
    p->setOpacity(appear);

    // 卡面
    QLinearGradient face(card.topLeft(), card.bottomLeft());
    face.setColorAt(0.0, QColor(196, 34, 44));
    face.setColorAt(0.55, QColor(166, 20, 32));
    face.setColorAt(1.0, QColor(118, 12, 24));
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 190), 2.2));
    p->setBrush(face);
    p->drawRoundedRect(card, 16.0, 16.0);

    // 可选卡面图：铺满卡面后再压一层红雾，保证上面几行金字依然清楚
    if (!m_cardImg.isNull()) {
        p->save();
        QPainterPath clip;
        clip.addRoundedRect(card, 16.0, 16.0);
        p->setClipPath(clip);
        p->setOpacity(0.62);
        p->drawPixmap(card, m_cardImg, QRectF(m_cardImg.rect()));
        p->setOpacity(1.0);
        p->setPen(Qt::NoPen);
        p->setBrush(Theme::withAlpha(QColor(132, 14, 26), 128));
        p->drawRect(card);
        p->restore();
        p->setPen(QPen(Theme::withAlpha(Theme::gold(), 190), 2.2));
        p->setBrush(Qt::NoBrush);
        p->drawRoundedRect(card, 16.0, 16.0);
    }

    // 内描金线
    p->setPen(QPen(Theme::withAlpha(Theme::gold(), 120), 1.2));
    p->setBrush(Qt::NoBrush);
    p->drawRoundedRect(card.adjusted(12.0, 12.0, -12.0, -12.0), 10.0, 10.0);

    // 主题句
    p->setFont(Theme::font(42, true));
    Theme::drawGlowText(p, QPointF(card.center().x(), card.top() + 78.0),
                        QStringLiteral("以我代码，贺你华诞"),
                        Theme::lightGold(), Theme::withAlpha(Theme::gold(), 120), 8.0);

    p->setFont(Theme::font(15));
    p->setPen(Theme::withAlpha(Theme::cream(), 210));
    p->drawText(QRectF(card.left(), card.top() + 106.0, card.width(), 22.0), Qt::AlignCenter,
                QStringLiteral("留校不是缺席团圆，而是把小家的思念写进大家的代码里"));

    p->setFont(Theme::font(12));
    p->setPen(Theme::withAlpha(Theme::cream(), 160));
    p->drawText(QRectF(card.left(), card.bottom() - 34.0, card.width(), 20.0), Qt::AlignCenter,
                QStringLiteral("2026 · 国庆七十七华诞 · 中秋月圆 · 安阳"));

    // 金色印章
    const QPointF seal(card.right() - 44.0, card.bottom() - 40.0);
    p->setBrush(Theme::withAlpha(Theme::gold(), 220));
    p->drawRoundedRect(QRectF(seal.x() - 22.0, seal.y() - 22.0, 44.0, 44.0), 6.0, 6.0);
    p->setPen(Theme::deepRed());
    p->setFont(Theme::font(14, true));
    p->drawText(QRectF(seal.x() - 22.0, seal.y() - 20.0, 22.0, 40.0), Qt::AlignCenter, QStringLiteral("团"));
    p->drawText(QRectF(seal.x() + 0.0, seal.y() - 20.0, 22.0, 40.0), Qt::AlignCenter, QStringLiteral("圆"));

    p->restore();
}

void EndingCard::drawWords(QPainter *p) const
{
    if (m_words.isEmpty())
        return;

    const qreal appear = Theme::easeOutCubic(Theme::clamp01((m_time - 0.9) / 0.8));
    p->save();
    p->setOpacity(appear);

    p->setFont(Theme::font(20, true));
    const qreal chipW = 92.0;
    const qreal gap = 14.0;
    const qreal totalW = chipW * m_words.size() + gap * qreal(m_words.size() - 1);
    qreal x = (qreal(Theme::kSceneW) - totalW) / 2.0;

    for (int i = 0; i < m_words.size(); ++i) {
        const qreal lift = 5.0 * std::sin(m_time * 2.0 + i * 0.7);
        const QRectF chip(x, 530.0 + lift, chipW, 46.0);
        p->setPen(QPen(Theme::withAlpha(Theme::gold(), 170), 1.4));
        p->setBrush(Theme::withAlpha(QColor(14, 18, 34), 200));
        p->drawRoundedRect(chip, 10.0, 10.0);
        p->setPen(Theme::lightGold());
        p->drawText(chip, Qt::AlignCenter, m_words.at(i));
        x += chipW + gap;
    }

    p->setFont(Theme::font(15));
    p->setPen(Theme::withAlpha(Theme::cream(), 200));
    p->drawText(QRectF(0.0, 584.0, qreal(Theme::kSceneW), 22.0), Qt::AlignCenter,
                QStringLiteral("本局得分 %1").arg(m_score));
    p->restore();
}

void EndingCard::drawButtons(QPainter *p) const
{
    Theme::drawButton(p, replayButtonRect(), QStringLiteral("再玩一次"), m_hoverReplay, m_pressReplay);
    Theme::drawButton(p, backButtonRect(), QStringLiteral("回到主场景"), m_hoverBack, m_pressBack, false);

    p->setFont(Theme::font(12));
    p->setPen(Theme::withAlpha(Theme::cream(), 130));
    p->drawText(QRectF(0.0, 674.0, qreal(Theme::kSceneW), 20.0), Qt::AlignCenter,
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
