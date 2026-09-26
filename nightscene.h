#ifndef NIGHTSCENE_H
#define NIGHTSCENE_H

/*
 * nightscene.h
 * 主场景：软件学院夜景 + 月亮投影 + 代码粒子，基于 QGraphicsScene / QGraphicsView 管理。
 *
 * 交互约定（由 NightScene 转发鼠标事件到 NightBackdrop）：
 *   点击月饼        -> 月亮像投影仪一样映出安阳（小家团圆）
 *   点击电脑        -> 代码字符化作金色粒子，汇成黄河 / 长城 / 高铁 / 航天（大家昌盛）
 *   拖动月亮        -> 切换投影场景
 *   点击右下按钮    -> 进入接月饼小游戏
 */

#include "theme.h"

#include <QElapsedTimer>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainterPath>
#include <QPixmap>
#include <QPointF>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVector>

class NightBackdrop : public QGraphicsObject
{
    Q_OBJECT

public:
    enum HitRole {
        HitNone = 0,
        HitMoon,
        HitMooncake,
        HitLaptop,
        HitStart
    };
    Q_ENUM(HitRole)

    enum Projection {
        NoProjection = -1,
        AnyangHome = 0,
        OracleGuo = 1,
        CampusMemory = 2
    };
    Q_ENUM(Projection)

    explicit NightBackdrop(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    void tick(qreal dt);           // 由 60 帧定时器驱动
    void reset();

    int hitTest(const QPointF &scenePos) const;
    void setPointer(const QPointF &scenePos);
    void beginDrag(const QPointF &scenePos);
    void moveDrag(const QPointF &scenePos);
    void endDrag();
    void trigger(HitRole role);

    // QPropertyAnimation 驱动的月亮属性
    Q_PROPERTY(QPointF moonPos READ moonPos WRITE setMoonPos NOTIFY moonChanged)
    Q_PROPERTY(qreal moonGlow READ moonGlow WRITE setMoonGlow NOTIFY moonChanged)

    QPointF moonPos() const { return m_moonPos; }
    void setMoonPos(const QPointF &pos);
    qreal moonGlow() const { return m_moonGlow; }
    void setMoonGlow(qreal glow);

    QRectF moonHandleRect() const;
    QRectF mooncakeRect() const;
    QRectF laptopRect() const;
    QRectF startButtonRect() const;
    QRectF screenRect() const;

    int projection() const { return m_proj; }
    void showProjection(int index);
    void clearProjection() { showProjection(NoProjection); }
    void riseMoon();       // 开场：月亮升起动画

signals:
    void moonChanged();
    void requestGame();
    void hintChanged(const QString &text);

private:
    struct Star { QPointF pos; qreal size; qreal phase; };
    struct Particle { QPointF from; QPointF to; QChar ch; qreal delay; qreal dur; qreal size; qreal wobble; };

    void ensureShapes(int index);
    const QVector<QPointF> &shapeTargets(int index) const;
    void startShape(int index);
    void rebuildParticles();

    // 分层绘制
    void drawPhotoBackdrop(QPainter *p) const;   // images/school_night.jpg（可选）
    void drawSky(QPainter *p) const;
    void drawStars(QPainter *p) const;
    void drawRemoteCity(QPainter *p) const;
    void drawCampusBuilding(QPainter *p) const;
    void drawGround(QPainter *p) const;
    void drawDesk(QPainter *p) const;
    void drawMoon(QPainter *p) const;
    void drawProjection(QPainter *p) const;
    void drawCodeParticles(QPainter *p) const;
    void drawHud(QPainter *p) const;

    // 可复用素材绘制
    static void paintWenfengTower(QPainter *p, const QRectF &rect, const QColor &line);
    static void paintOracleGlyph(QPainter *p, const QRectF &rect, int glyph, const QColor &line);
    static void paintYard(QPainter *p, const QRectF &rect, const QColor &line);
    static void paintFiveStars(QPainter *p, const QRectF &rect, const QColor &line);
    static void paintCampusTower(QPainter *p, const QRectF &rect, const QColor &line);
    static void paintBike(QPainter *p, const QPointF &base, qreal scale, const QColor &line);
    static void paintFigure(QPainter *p, const QPointF &feet, qreal height, const QColor &fill);
    static void paintTree(QPainter *p, const QPointF &base, qreal height, const QColor &fill);

    // 静态布局
    static QVector<QRectF> facadeWindows();

    qreal m_time = 0.0;

    // 月亮
    QPointF m_moonPos = QPointF(1010.0, 158.0);
    qreal m_moonGlow = 0.82;
    bool m_dragging = false;
    QPointF m_dragOffset;
    qreal m_switchAnchor = 0.0;
    bool m_moonPressed = false;

    // 投影
    int m_proj = NoProjection;
    qreal m_projAlpha = 0.0;
    qreal m_projTime = 0.0;

    // 代码粒子
    bool m_codeRunning = false;
    int m_shapeIndex = -1;
    qreal m_shapeTime = 0.0;
    QVector<Particle> m_particles;
    mutable QVector<QVector<QPointF>> m_shapeCache;
    mutable QVector<QPointF> m_empty;

    // 场景元素
    QVector<Star> m_stars;
    QVector<QRectF> m_windows;
    QVector<qreal> m_winPhase;
    QVector<int> m_winTone;

    // 交互反馈
    int m_hoverRole = HitNone;
    bool m_pressing = false;
    int m_pressRole = HitNone;
    qreal m_pulse = 0.0;

    QPixmap m_moonImg;
    QPixmap m_cakeImg;
    QPixmap m_sparkImg;
    QPixmap m_bgImg;     // 可选：images/school_night.jpg —— 有则当作写实底图
    QPixmap m_projImg;   // 可选：images/anyang_projection.jpg —— 安阳文峰塔，贴进「小家团圆」投影圆幕
    QPixmap m_yinxuImg;  // 可选：images/yinxu_projection.jpg —— 殷墟博物馆夜景，贴进「甲骨·国」投影圆幕
};

class NightScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit NightScene(QObject *parent = nullptr);
    ~NightScene() override;

    NightBackdrop *backdrop() const { return m_backdrop; }
    void startClock();
    void stopClock();

signals:
    void requestGame();
    void requestStart();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTick();

private:
    NightBackdrop *m_backdrop = nullptr;
    QTimer m_timer;
    QElapsedTimer m_clock;
};

class NightSceneView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit NightSceneView(NightScene *scene, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateTransform();
};

#endif // NIGHTSCENE_H
