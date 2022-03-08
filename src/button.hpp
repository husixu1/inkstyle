#ifndef BUTTON_H
#define BUTTON_H

#include <QEvent>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRegion>

class Button : public QPushButton {
    Q_OBJECT

public:
    Button(
        QRect geometry, QPolygonF mask, qreal hoverScale, QPointF centroid,
        QPointF bgOffset = QPointF(0, 0), QWidget *parent = nullptr);

    const QColor &getBgColor() const;
    void setBgColor(const QColor &newBgColor);

protected:
    virtual void enterEvent(QEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;

    /// @brief Overriden to set mask on every resize
    virtual void resizeEvent(QResizeEvent *e) override;

    virtual void paintEvent(QPaintEvent *e) override;

signals:
    void mouseEnter();
    void mouseLeave();

private:
    const QRect inactiveGeometry;
    const QPolygonF inactiveMask;
    const qreal hoverScale;
    /// @brief The geometry center of the background
    const QPointF centroid;
    /// @brief For calibrating the sub-pixel position of the background
    const QPointF bgOffset;
    const QColor inactiveBgColor;
    const QColor activeBgColor;

    QPoint mousePos;
    bool hovering;

    QColor bgColor;
    Q_PROPERTY(QColor bgColor READ getBgColor WRITE setBgColor)

    QParallelAnimationGroup animations;
    QPropertyAnimation geometryAnimation;
    QPropertyAnimation bgColorAnimation;
    void startAnimation();
};

#endif // BUTTON_H
