#ifndef BUTTON_H
#define BUTTON_H

#include <QEvent>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRegion>

class Button : public QPushButton {
    Q_OBJECT

private:
    const QRect inactiveGeometry;
    const QPolygonF inactiveMask;
    const qreal hoverScale;
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

public:
    Button(
        QRect geometry, QPolygonF mask, qreal hoverScale, QPointF bgOffset,
        QWidget *parent = nullptr);

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
};

#endif // BUTTON_H
