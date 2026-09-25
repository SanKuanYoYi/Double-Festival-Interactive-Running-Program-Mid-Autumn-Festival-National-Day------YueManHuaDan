/*
 * mainwindow.cpp
 * 启动页 + 页面路由（启动页 / 主场景 / 小游戏 / 结尾）。
 */

#include "mainwindow.h"

#include "soundfx.h"

#include <QFontMetrics>
#include <QGraphicsView>
#include <QLinearGradient>
#include <QPainter>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QtGlobal>

#include <cmath>

namespace {

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
/*  StartPage                                                                 */
/* ========================================================================== */
StartPage::StartPage(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_words = QStringList{ QStringLiteral("团圆"), QStringLiteral("昌盛"),
                           QStringLiteral("华诞"), QStringLiteral("厚德"),
                           QStringLiteral("博学"), QStringLiteral("至善") };

    quint32 seed = 20260923u;
    auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };
    for (int i = 0; i < 130; ++i) {
        Star s;
        s.pos = QPointF(qreal(next() % 128000u) / 100.0, qreal(next() % 66000u) / 100.0);
        s.size = 0.7 + qreal(next() % 180u) / 100.0;
        s.phase = qreal(next() % 628u) / 100.0;
        m_stars << s;
    }

    updateTransform();
    connect(&m_timer, &QTimer::timeout, this, &StartPage::tick);
    m_timer.start(16);
    m_clock.start();
}

void StartPage::updateTransform()
{
    const qreal sx = width() > 0 ? width() / qreal(Theme::kSceneW) : 1.0;
    const qreal sy = height() > 0 ? height() / qreal(Theme::kSceneH) : 1.0;
    m_scale = qMin(sx, sy);
    if (m_scale <= 0.0)
        m_scale = 1.0;
    m_offsetX = (width() - qreal(Theme::kSceneW) * m_scale) / 2.0;
    m_offsetY = (height() - qreal(Theme::kSceneH) * m_scale) / 2.0;
}

QPointF StartPage::toLogical(const QPointF &widgetPos) const
{
    return QPointF((widgetPos.x() - m_offsetX) / m_scale, (widgetPos.y() - m_offsetY) / m_scale);
}

void StartPage::tick()
{
    const qint64 elapsed = m_clock.restart();
    m_time += qMin(qreal(elapsed) / 1000.0, 0.05);
    update();
}

void StartPage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    updateTransform();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.translate(m_offsetX, m_offsetY);
    p.scale(m_scale, m_scale);

    /* 夜空 */
    QLinearGradient sky(0.0, 0.0, 0.0, qreal(Theme::kSceneH));
    sky.setColorAt(0.0, Theme::nightTop());
    sky.setColorAt(0.52, QColor(15, 21, 44));
    sky.setColorAt(1.0, Theme::nightBottom());
    p.setPen(Qt::NoPen);
    p.setBrush(sky);
    p.drawRect(QRectF(0.0, 0.0, qreal(Theme::kSceneW), qreal(Theme::kSceneH)));

    /* 星 */
    for (const Star &s : m_stars) {
        const qreal tw = 0.35 + 0.65 * std::sin(m_time * 1.5 + s.phase);
        p.setBrush(Theme::withAlpha(Theme::warmWhite(), int(210.0 * tw)));
        p.drawEllipse(s.pos, s.size, s.size);
    }

    /* 月 */
    const QPointF moon(640.0, 208.0);
    QRadialGradient halo(moon, 300.0);
    halo.setColorAt(0.0, Theme::withAlpha(Theme::lightGold(), 110));
    halo.setColorAt(0.35, Theme::withAlpha(Theme::gold(), 34));
    halo.setColorAt(1.0, Theme::withAlpha(Theme::gold(), 0));
    p.setBrush(halo);
    p.drawEllipse(moon, 300.0, 300.0);

    QRadialGradient body(QPointF(moon.x() - 26.0, moon.y() - 26.0), 130.0);
    body.setColorAt(0.0, Theme::cream());
    body.setColorAt(0.72, Theme::mixColor(Theme::cream(), Theme::gold(), 0.35));
    body.setColorAt(1.0, Theme::mixColor(Theme::gold(), Theme::cream(), 0.25));
    p.setBrush(body);
    p.drawEllipse(moon, 92.0, 92.0);
    p.setBrush(Theme::withAlpha(Theme::gold(), 40));
    for (int i = 0; i < 6; ++i) {
        const qreal a = i * 1.047;
        p.drawEllipse(QPointF(moon.x() + std::cos(a) * 44.0, moon.y() + std::sin(a) * 40.0),
                      13.0 + (i % 3) * 3.0, 12.0 + (i % 2) * 4.0);
    }

    /* 六个词绕月浮动 */
    p.setFont(Theme::font(16, true));
    const QFontMetrics fm(p.font());
    for (int i = 0; i < m_words.size(); ++i) {
        const qreal base = qreal(i) / qreal(m_words.size()) * 2.0 * M_PI + m_time * 0.18;
        const qreal rx = 168.0 + 12.0 * std::sin(m_time * 1.1 + i);
        const qreal ry = 96.0;
        const QPointF c(moon.x() + std::cos(base) * rx, moon.y() + std::sin(base) * ry
                        + 8.0 * std::sin(m_time * 1.7 + i));
        const qreal w = fm.horizontalAdvance(m_words.at(i)) + 26.0;
        const QRectF chip(c.x() - w / 2.0, c.y() - 17.0, w, 34.0);
        p.setPen(QPen(Theme::withAlpha(Theme::gold(), 150), 1.2));
        p.setBrush(Theme::withAlpha(QColor(12, 16, 32), 190));
        p.drawRoundedRect(chip, 17.0, 17.0);
        p.setPen(Theme::lightGold());
        p.drawText(chip, Qt::AlignCenter, m_words.at(i));
    }

    /* 标题 */
    p.setFont(Theme::font(54, true));
    Theme::drawGlowText(&p, QPointF(640.0, 404.0), QStringLiteral("月满华诞，码上团圆"),
                        Theme::cream(), Theme::withAlpha(Theme::gold(), 130), 10.0);

    p.setPen(QPen(Theme::withAlpha(Theme::gold(), 160), 2.0));
    p.drawLine(QPointF(500.0, 442.0), QPointF(780.0, 442.0));

    p.setFont(Theme::font(17));
    p.setPen(Theme::withAlpha(Theme::cream(), 210));
    p.drawText(QRectF(0.0, 458.0, qreal(Theme::kSceneW), 24.0), Qt::AlignCenter,
               QStringLiteral("留校不是缺席团圆，而是把小家的思念，放进软件学院的大家里"));

    p.setFont(Theme::font(13));
    p.setPen(Theme::withAlpha(Theme::cream(), 150));
    p.drawText(QRectF(0.0, 486.0, qreal(Theme::kSceneW), 20.0), Qt::AlignCenter,
               QStringLiteral("2026 · 国庆七十七华诞 · 中秋 · 安阳 · 软件学院"));

    /* 开始按钮 */
    Theme::drawButton(&p, startButtonRect(), QStringLiteral("开 始"), m_hoverStart, m_pressStart);

    /* 说明 */
    p.setFont(Theme::font(13));
    p.setPen(Theme::withAlpha(Theme::cream(), 165));
    p.drawText(QRectF(0.0, 606.0, qreal(Theme::kSceneW), 20.0), Qt::AlignCenter,
               QStringLiteral("点击月饼看安阳 · 点击电脑看祝福 · 拖动月亮换场景 · 接月饼点亮全楼"));
    p.setFont(Theme::font(15, true));
    p.setPen(Theme::withAlpha(Theme::lightGold(), 200));
    p.drawText(QRectF(0.0, 652.0, qreal(Theme::kSceneW), 22.0), Qt::AlignCenter,
               QStringLiteral("以我代码，贺你华诞"));
}

void StartPage::mouseMoveEvent(QMouseEvent *event)
{
    m_pointer = toLogical(QPointF(mousePoint(event)));
    m_hoverStart = startButtonRect().contains(m_pointer);
    update();
    QWidget::mouseMoveEvent(event);
}

void StartPage::mousePressEvent(QMouseEvent *event)
{
    m_pointer = toLogical(QPointF(mousePoint(event)));
    m_pressStart = startButtonRect().contains(m_pointer);
    update();
    QWidget::mousePressEvent(event);
}

void StartPage::mouseReleaseEvent(QMouseEvent *event)
{
    m_pointer = toLogical(QPointF(mousePoint(event)));
    if (m_pressStart && startButtonRect().contains(m_pointer)) {
        SoundFx::playClick();
        emit startRequested();
    }
    m_pressStart = false;
    update();
    QWidget::mouseReleaseEvent(event);
}

void StartPage::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space)
        emit startRequested();
    QWidget::keyPressEvent(event);
}

void StartPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateTransform();
}

/* ========================================================================== */
/*  MainWindow                                                                */
/* ========================================================================== */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("月满华诞 · 码上团圆 | YueManHuaDan"));
    resize(Theme::kSceneW, Theme::kSceneH);
    setMinimumSize(960, 540);

    m_stack = new QStackedWidget(this);

    m_start = new StartPage(m_stack);
    m_scene = new NightScene(this);
    m_view = new NightSceneView(m_scene, m_stack);
    m_game = new GameWidget(m_stack);
    m_ending = new EndingCard(m_stack);

    m_stack->addWidget(m_start);
    m_stack->addWidget(m_view);
    m_stack->addWidget(m_game);
    m_stack->addWidget(m_ending);

    setCentralWidget(m_stack);

    connect(m_start, &StartPage::startRequested, this, &MainWindow::enterScene);
    connect(m_scene, &NightScene::requestGame, this, &MainWindow::enterGame);
    connect(m_scene, &NightScene::requestStart, this, &MainWindow::backToStart);
    connect(m_game, &GameWidget::gameFinished, this, &MainWindow::showEnding);
    connect(m_game, &GameWidget::backRequested, this, &MainWindow::backToScene);
    connect(m_ending, &EndingCard::replayRequested, this, &MainWindow::enterGame);
    connect(m_ending, &EndingCard::backRequested, this, &MainWindow::backToScene);

    m_stack->setCurrentWidget(m_start);
    setFocusPolicy(Qt::StrongFocus);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (m_stack->currentWidget() != m_start)
            backToStart();
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::enterScene()
{
    m_stack->setCurrentWidget(m_view);
    if (m_scene && m_scene->backdrop())
        m_scene->backdrop()->reset();
    m_scene->startClock();
    m_view->setFocus();
}

void MainWindow::enterGame()
{
    m_game->restart();
    m_stack->setCurrentWidget(m_game);
    m_scene->stopClock();
    m_game->setFocus();
}

void MainWindow::showEnding(int score, const QStringList &words)
{
    m_ending->setResult(score, words);
    m_stack->setCurrentWidget(m_ending);
    m_ending->setFocus();
}

void MainWindow::backToScene()
{
    m_stack->setCurrentWidget(m_view);
    m_scene->startClock();
    m_view->setFocus();
}

void MainWindow::backToStart()
{
    m_stack->setCurrentWidget(m_start);
    m_scene->stopClock();
    m_start->setFocus();
}
