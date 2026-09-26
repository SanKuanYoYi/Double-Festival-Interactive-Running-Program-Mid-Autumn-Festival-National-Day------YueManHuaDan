#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

/*
 * gamewidget.h
 * 接月饼小游戏：月饼下落 -> 鼠标托盘接住 -> 每接住一个点亮一扇留校窗户。
 * 集齐六个词后：窗户先组成「77」，再演变为中国地图形状，最后进入结尾。
 *
 * 碰撞检测使用 QRectF::intersects()，月饼集合使用 QVector<Mooncake>。
 */

#include "theme.h"

#include <QElapsedTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <QWidget>

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);

signals:
    void gameFinished(int score, const QStringList &words);
    void backRequested();

public slots:
    void restart();

public:
    // 演示 / 截图用（配合 main.cpp 的 --shots）：
    // 直接把窗户点阵跳到某一幕并「全部点亮」，随后冻结自动推进，方便抓图。
    // 1 = 窗户拼出「77」；2 = 窗户拼出「国」。传其它值不做任何事。
    void demoForcePhase(int phase);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Mooncake
    {
        QRectF rect;
        QString word;
        qreal vy = 170.0;
        qreal rot = 0.0;
        qreal vr = 0.0;
    };

    struct Puff
    {
        QPointF pos;
        qreal life = 1.0;
    };

    struct Cell
    {
        int col = 0;
        int row = 0;
    };

    enum Phase { Playing, RevealSeven, MorphGuo, Finished };

    void tick();
    void spawn();
    void buildPatterns();
    void updateCells(qreal dt);
    void lightNextCell();
    void ensureCache();
    void updateTransform();

    QRectF basketRect() const;
    QRectF backButtonRect() const;
    QRectF windowRect(int col, int row) const;
    QPointF toLogical(const QPointF &widgetPos) const;

    void drawBackground(QPainter *p) const;
    void drawWindows(QPainter *p) const;
    void drawCakes(QPainter *p) const;
    void drawBasket(QPainter *p) const;
    void drawHud(QPainter *p) const;

    // 逻辑横向竞争数据
    QVector<Mooncake> m_cakes;
    QVector<Puff> m_puffs;
    QVector<Cell> m_sevenCells;
    QVector<Cell> m_guoCells;

    QStringList m_words;
    QVector<int> m_wordCount;
    int m_needPerWord = 2;

    Phase m_phase = Playing;
    qreal m_time = 0.0;
    qreal m_phaseTime = 0.0;
    qreal m_spawnTimer = 0.0;
    qreal m_spawnInterval = 0.78;
    int m_score = 0;
    int m_caught = 0;
    int m_missed = 0;
    int m_sevenIndex = 0;
    int m_guoIndex = 0;
    qreal m_revealAcc = 0.0;
    bool m_completed = false;
    bool m_demoHold = false;   // 演示截图时冻结「77 → 国 → 结尾」的自动推进

    QVector<qreal> m_cellBright;
    QVector<qreal> m_cellTarget;
    QVector<qreal> m_cellPop;

    QPointF m_pointer;
    qreal m_basketX = qreal(Theme::kSceneW) / 2.0;
    bool m_pointerInside = false;

    int m_hoverBack = 0;
    bool m_pressingBack = false;

    QTimer m_timer;
    QElapsedTimer m_clock;

    qreal m_scale = 1.0;
    qreal m_offsetX = 0.0;
    qreal m_offsetY = 0.0;

    QPixmap m_base;
    QPixmap m_cakeImg;
    QPixmap m_sparkImg;
    QPixmap m_bgImg;     // images/school_night.jpg —— 楼前广场写实底图
    QPixmap m_moonImg;   // images/moon.png         —— 与其余场景共用的同一轮月亮
    bool m_cacheReady = false;
};

#endif // GAMEWIDGET_H
