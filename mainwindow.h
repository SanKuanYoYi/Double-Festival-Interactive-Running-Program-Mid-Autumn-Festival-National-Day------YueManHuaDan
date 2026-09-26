#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/*
 * mainwindow.h
 * 主窗口：以 QStackedWidget 串起「启动页 → 主场景 → 接月饼小游戏 → 结尾」。
 */

#include "theme.h"

#include "endingcard.h"
#include "gamewidget.h"
#include "nightscene.h"

#include <QElapsedTimer>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QStackedWidget>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <QWidget>

/* ---------------- 启动页 ---------------- */
class StartPage : public QWidget
{
    Q_OBJECT

public:
    explicit StartPage(QWidget *parent = nullptr);

signals:
    void startRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Star
    {
        QPointF pos;
        qreal size;
        qreal phase;
    };

    void tick();
    void updateTransform();
    QPointF toLogical(const QPointF &widgetPos) const;
    QRectF startButtonRect() const { return QRectF(520.0, 498.0, 240.0, 58.0); }

    QVector<Star> m_stars;
    QStringList m_words;
    qreal m_time = 0.0;
    bool m_hoverStart = false;
    bool m_pressStart = false;
    QPointF m_pointer;

    QPixmap m_bgImg;   // images/school_night.jpg —— 启动页写实底图
    QPixmap m_moonImg; // images/moon.png —— 与主场景共用的同一轮月亮

    QTimer m_timer;
    QElapsedTimer m_clock;

    qreal m_scale = 1.0;
    qreal m_offsetX = 0.0;
    qreal m_offsetY = 0.0;
};

/* ---------------- 主窗口 ---------------- */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;

public slots:
    void enterScene();
    void enterGame();
    void showEnding(int score, const QStringList &words);
    void backToScene();
    void backToStart();

    // 演示 / 截图用接口
    NightScene *scene() const { return m_scene; }
    GameWidget *game() const { return m_game; }

private:
    QStackedWidget *m_stack = nullptr;
    StartPage *m_start = nullptr;
    NightScene *m_scene = nullptr;
    NightSceneView *m_view = nullptr;
    GameWidget *m_game = nullptr;
    EndingCard *m_ending = nullptr;
};

#endif // MAINWINDOW_H
