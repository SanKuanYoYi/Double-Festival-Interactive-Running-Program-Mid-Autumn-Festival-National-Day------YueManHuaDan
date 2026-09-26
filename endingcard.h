#ifndef ENDINGCARD_H
#define ENDINGCARD_H

/*
 * endingcard.h
 * 结尾：校训石亮起「厚德博学，止于至善」，祝福卡写下「以我代码，贺你华诞」。
 * 底图为校训楼夜色实拍（images/ending_card.jpg），右下角缀原图书法「楼倚暮霞 德韵扬辉」。
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
    QRectF replayButtonRect() const { return QRectF(438.0, 502.0, 190.0, 52.0); }
    QRectF backButtonRect() const { return QRectF(652.0, 502.0, 190.0, 52.0); }

    void drawBackground(QPainter *p) const;
    void drawMotto(QPainter *p) const;
    void drawGreetingCard(QPainter *p) const;
    void drawWords(QPainter *p) const;
    void drawCalligraphy(QPainter *p) const;
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

    QPixmap m_bgImg;      // images/ending_card.jpg    —— 校训楼夜色实拍底图
    QPixmap m_mottoImg;   // images/ending_motto.png   —— 原图书法「楼倚暮霞 德韵扬辉」
    QPixmap m_moonImg;    // images/moon.png           —— 与主场景共用的同一轮月亮

    qreal m_scale = 1.0;
    qreal m_offsetX = 0.0;
    qreal m_offsetY = 0.0;
};

#endif // ENDINGCARD_H
