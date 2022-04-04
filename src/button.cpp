#include "button.hpp"

#include "constants.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QTransform>
#include <iostream>

Button::Button(
    QRectF geometry, QPolygonF maskPolygon, qreal hoverScale, QPointF centroid,
    QPointF bgOffset, QWidget *parent, QColor inactiveColor, QColor activeColor)
    : QPushButton(parent), inactiveGeometry(geometry),
      inactiveMask(maskPolygon), hoverScale(hoverScale), centroid(centroid),
      bgOffset(bgOffset), inactiveBgColor(inactiveColor),
      activeBgColor(activeColor), hovering(false), active(false),
      bgColor(inactiveBgColor), animations(),
      geometryAnimation(this, "geometry"), bgColorAnimation(this, "bgColor") {
    Q_ASSERT(hoverScale > 1.);

    setGeometry(geometry.toRect());
    setMask(maskPolygon.toPolygon());

    // initialize animation
    geometryAnimation.setDuration(120);
    geometryAnimation.setStartValue(inactiveGeometry);
    animations.addAnimation(&geometryAnimation);

    bgColorAnimation.setDuration(120);
    bgColorAnimation.setStartValue(inactiveBgColor);
    animations.addAnimation(&bgColorAnimation);

    // Make this button toggle-able
    connect(this, &QPushButton::clicked, [this] {
        active = !active;
        startAnimation();
    });
}

void Button::enterEvent(QEvent *) {
    hovering = true;
    startAnimation();
    update();
    emit mouseEnter();
}

void Button::leaveEvent(QEvent *) {
    hovering = false;
    startAnimation();
    update();
    emit mouseLeave();
}

void Button::mouseMoveEvent(QMouseEvent *) {}

void Button::resizeEvent(QResizeEvent *e) {
    // transform icon
    setIconSize(e->size());

    // Transform the mask. We paint the background polygon explicitly and offset
    // the mask by 2px so that the background edge can get antialiased. (There's
    // no antialias effect if we only mask the button with QRegion)
    QTransform transform =
        QTransform::fromScale(
            qreal(e->size().width() + 4) / qreal(inactiveGeometry.width()),
            qreal(e->size().height() + 4) / qreal(inactiveGeometry.height()))
        * QTransform::fromTranslate(-2, -2);
    setMask((inactiveMask * transform).toPolygon());
}

void Button::paintEvent(QPaintEvent *e) {
    QPainter painter(this);

    painter.setRenderHints(
        QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
    painter.setPen(bgColor);
    painter.setBrush(bgColor);

    // Paint the background polygon explicitly
    QTransform transform =
        QTransform::fromScale(
            qreal(width()) / qreal(inactiveGeometry.width()),
            qreal(height()) / qreal(inactiveGeometry.height()))
        * QTransform::fromTranslate(bgOffset.x(), bgOffset.y());
    painter.drawPolygon(inactiveMask * transform);

    // Let parent object paint icons
    QPushButton::paintEvent(e);
}

bool Button::isActive() const {
    return active;
}

const QColor &Button::getBgColor() const {
    return bgColor;
}

void Button::setBgColor(const QColor &newBgColor) {
    bgColor = newBgColor;
}

void Button::startAnimation() {
    animations.stop();
    geometryAnimation.setStartValue(geometryAnimation.currentValue());
    bgColorAnimation.setStartValue(bgColorAnimation.currentValue());
    if (hovering || active) {
        raise();
        QPointF centroidOffset = centroid * (hoverScale - 1.);
        geometryAnimation.setEndValue(
            QRectF(
                inactiveGeometry.topLeft() - centroidOffset,
                inactiveGeometry.size() * hoverScale)
                .toRect());
        bgColorAnimation.setEndValue(activeBgColor);
    } else {
        lower();
        geometryAnimation.setEndValue(inactiveGeometry);
        bgColorAnimation.setEndValue(inactiveBgColor);
    }
    animations.start();
}
