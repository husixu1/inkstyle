#include "button.h"

#include <QMouseEvent>
#include <QResizeEvent>
#include <QTransform>
#include <iostream>

Button::Button(QRect geometry, QRegion mask, qreal hoverScale, QWidget *parent)
    : QPushButton(parent), inactiveGeometry(geometry), inactiveMask(mask),
      hovering(false), animation(this, "geometry"), hoverScale(hoverScale) {
    assert(hoverScale > 1.);

    setGeometry(geometry);
    setMask(mask);

    // initialize animation
    animation.setDuration(120);
    animation.setStartValue(geometry);
}

void Button::enterEvent(QEvent *e) {
    hovering = true;
    startAnimation();
    update();
    emit mouseEnter();
}

void Button::leaveEvent(QEvent *e) {
    hovering = false;
    startAnimation();
    update();
    emit mouseLeave();
}

void Button::mouseMoveEvent(QMouseEvent *e) {
    // std::cout << this << ": " << e->pos().x() << ", " << e->pos().y()
    //           << std::endl;
    // repaint();
}

void Button::resizeEvent(QResizeEvent *e) {
    QTransform transform = QTransform::fromScale(
        qreal(e->size().width()) / qreal(inactiveGeometry.width()),
        qreal(e->size().height()) / qreal(inactiveGeometry.height()));
    setMask(inactiveMask * transform);
}

void Button::paintEvent(QPaintEvent *e) {
    // set geometry (coordinate, size)

    // trasnform mask

    QPushButton::paintEvent(e);
}

void Button::startAnimation() {
    animation.stop();
    animation.setStartValue(animation.currentValue());
    if (hovering) {
        raise();
        animation.setEndValue(QRect(
            inactiveGeometry.x()
                - inactiveGeometry.width() * (hoverScale - 1.) / 2.,
            inactiveGeometry.y()
                - inactiveGeometry.height() * (hoverScale - 1.) / 2.,
            inactiveGeometry.width() * hoverScale,
            inactiveGeometry.height() * hoverScale));
    } else {
        lower();
        animation.setEndValue(inactiveGeometry);
    }
    animation.start();
}
