#include "button.hpp"

#include "constants.hpp"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QTransform>
#include <iostream>

Button::Button(
    QRectF geometry, QPolygonF maskPolygon, qreal hoverScale, QPointF centroid,
    QWidget *parent, QColor inactiveColor, QColor activeColor)
    : QPushButton(parent), inactiveGeometry(geometry),
      inactiveMask(maskPolygon), hoverScale(hoverScale), centroid(centroid),
      bgOffset(geometry.topLeft() - geometry.toRect().topLeft()),
      inactiveBgColor(inactiveColor), activeBgColor(activeColor),
      hovering(false), leftClicked(false), rightClicked(false),
      bgColor(inactiveBgColor), activationAnimations(),
      geometryAnimation(this, "geometry"), bgColorAnimation(this, "bgColor"),
      updateProgress(0), updateAnimation(this, "updateProgress") {
    Q_ASSERT(hoverScale > 1.);

    setGeometry(geometry.toRect());
    setMask(maskPolygon.toPolygon());

    // initialize animation
    geometryAnimation.setDuration(120);
    geometryAnimation.setStartValue(inactiveGeometry);
    activationAnimations.addAnimation(&geometryAnimation);

    bgColorAnimation.setDuration(120);
    bgColorAnimation.setStartValue(inactiveBgColor);
    activationAnimations.addAnimation(&bgColorAnimation);

    // Force repaint during the update anmation since qt won't repaint this
    // automatically (there's no geometry update).
    updateAnimation.setDuration(1000);
    updateAnimation.setStartValue(0);
    connect(
        &updateAnimation, &QPropertyAnimation::valueChanged, this,
        QOverload<>::of(&Button::update));
}

void Button::enterEvent(QEvent *) {
    // Make sure event only triggered once
    if (hovering)
        return;
    hovering = true;
    restartActivationAnimation();
    emit mouseEnter();
}

void Button::leaveEvent(QEvent *) {
    // Make sure event only triggered once (sometimes leaveEvent triggers
    // indefinitely. Not sure why.)
    if (!hovering)
        return;
    hovering = false;
    restartActivationAnimation();
    emit mouseLeave();
}

void Button::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::RightButton) {
        rightClicked = true;
        connect(
            &updateAnimation, &QPropertyAnimation::finished, this,
            &Button::stateUpdated);
        restartUpdateAnimation();
    } else {
        // Propagate unhandeled event to parent class
        QPushButton::mousePressEvent(e);
    }
}

void Button::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::RightButton) {
        rightClicked = false;
        disconnect(
            &updateAnimation, &QPropertyAnimation::finished, nullptr, nullptr);
        restartUpdateAnimation();
    } else {
        // Propagate unhandeled event to parent class
        QPushButton::mouseReleaseEvent(e);
    }
}

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

    // Paint the border-highlighting for the update action
    if (updateProgress > 0) {
        qreal edgeLen = qMax(width(), height()) * 3;
        QPainterPath clipPath;
        clipPath.moveTo(centroid * transform);
        clipPath.arcTo(
            QRectF(
                centroid * transform - QPointF(0.5 * edgeLen, 0.5 * edgeLen),
                QSizeF(edgeLen, edgeLen)),
            90, -updateProgress * 360);
        clipPath.closeSubpath();

        painter.setClipPath(clipPath);
        painter.setPen(QPen(Qt::white, 5));
        painter.setBrush(Qt::transparent);
        painter.drawPolygon(inactiveMask * transform);
    }

    // Let parent object paint icons
    QPushButton::paintEvent(e);
}

bool Button::isActive() const {
    return leftClicked;
}

bool Button::isHovering() const {
    return hovering;
}

void Button::toggle() {
    leftClicked = !leftClicked;
    restartActivationAnimation();
}

const QColor &Button::getBgColor() const {
    return bgColor;
}

void Button::setBgColor(const QColor &newBgColor) {
    bgColor = newBgColor;
}

void Button::restartActivationAnimation() {
    activationAnimations.stop();
    geometryAnimation.setStartValue(geometryAnimation.currentValue());
    bgColorAnimation.setStartValue(bgColorAnimation.currentValue());
    if (hovering || leftClicked) {
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
    activationAnimations.start();
}

qreal Button::getUpdateProgress() const {
    return updateProgress;
}

void Button::setUpdateProgress(qreal newUpdateProgress) {
    if (qFuzzyCompare(updateProgress, newUpdateProgress))
        return;
    updateProgress = newUpdateProgress;
}

void Button::restartUpdateAnimation() {
    updateAnimation.stop();
    updateAnimation.setStartValue(updateAnimation.currentValue());
    if (rightClicked) {
        raise();
        updateAnimation.setEndValue(1.);
    } else {
        lower();
        updateAnimation.setEndValue(0.);
    }
    updateAnimation.start();
}
