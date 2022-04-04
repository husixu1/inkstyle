#ifndef BUTTON_H
#define BUTTON_H

#include "constants.hpp"

#include <QEvent>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRegion>
#include <QWeakPointer>

class Button : public QPushButton {
    Q_OBJECT

public:
    Button(
        QRectF geometry, QPolygonF mask, qreal hoverScale, QPointF centroid,
        QPointF bgOffset = QPointF(0, 0), QWidget *parent = nullptr,
        QColor inactiveColor = C::DBC::off, QColor activeColor = C::DBC::on);

    const QColor &getBgColor() const;
    void setBgColor(const QColor &newBgColor);

protected:
    virtual void enterEvent(QEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;

    /// @brief Overridden to set mask on every resize
    virtual void resizeEvent(QResizeEvent *e) override;

    virtual void paintEvent(QPaintEvent *e) override;

signals:
    void mouseEnter();
    void mouseLeave();

public:
    const QRectF inactiveGeometry;
    const QPolygonF inactiveMask;
    const qreal hoverScale;
    /// @brief The geometry center of the background
    const QPointF centroid;
    /// @brief For calibrating the sub-pixel position of the background
    const QPointF bgOffset;
    const QColor inactiveBgColor;
    const QColor activeBgColor;

private:
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
