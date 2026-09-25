#ifndef ENDINGCARD_H
#define ENDINGCARD_H

/*
 * endingcard.h
 * 结尾：校训石亮起「厚德博学，止于至善」，祝福卡写下「以我代码，贺你华诞」。
 */

#include "theme.h"

#include <QElapsedTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <QWidget>

class EndingCard : public QWidget
{
    Q_OBJECT

public:
    explicit EndingCard(QWidget *parent = nullptr);

    void setResult(int score, const QStringList &words);

signals:
    void replayRequested();
    void backRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Ember
    {
        QPointF pos;
        qreal vy;
        qreal size;
        qreal phase;
    };

    void tick();
    void updateTransform();
    QPointF toLogical(const QPointF &widgetPos) const;
    QRectF replayButtonRect() const { return QRectF(438.0, 612.0, 190.0, 52.0); }
    QRectF backButtonRect() const { return QRectF(652.0, 612.0, 190.0, 52.0); }

    void drawBackground(QPainter *p) const;
    void drawMotto(QPainter *p) const;
    void drawGreetingCard(QPainter *p) const;
    void drawWords(QPainter *p) const;
    void drawButtons(QPainter *p) const;

    int m_score = 0;
    QStringList m_words;
    qreal m_time = 0.0;
    QVector<Ember> m_embers;

    QPointF m_pointer;
    bool m_hoverReplay = false;
    bool m_hoverBack = false;
    bool m_pressReplay = false;
    bool m_pressBack = false;

    QTimer m_timer;
    QElapsedTimer m_clock;

    QPixmap m_cardImg;   // 可选：images/ending_card.png —— 有则当作祝福卡卡面

    qreal m_scale = 1.0;
    qreal m_offsetX = 0.0;
    qreal m_offsetY = 0.0;
};

#endif // ENDINGCARD_H
