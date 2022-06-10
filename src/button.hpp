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
        QWidget *parent = nullptr, QColor inactiveColor = C::DBC::off,
        QColor activeColor = C::DBC::on);

    bool isActive() const;
    bool isHovering() const;

public slots:
    void toggle();

protected:
    virtual void enterEvent(QEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;

    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;

    /// @brief Overridden to set mask on every resize
    virtual void resizeEvent(QResizeEvent *e) override;

    virtual void paintEvent(QPaintEvent *e) override;

signals:
    void mouseEnter();
    void mouseLeave();

    /// @brief This signal is be emitted at the end of rightclick & hold action
    void stateUpdated();

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
    bool leftClicked;
    bool rightClicked;

    QColor bgColor;
    const QColor &getBgColor() const;
    void setBgColor(const QColor &newBgColor);
    Q_PROPERTY(QColor bgColor READ getBgColor WRITE setBgColor)

    QParallelAnimationGroup activationAnimations;
    QPropertyAnimation geometryAnimation;
    QPropertyAnimation bgColorAnimation;
    /// @brief Restart the activation animation
    /// @details the activation animation includes resize the button and change
    /// the background color.
    void restartActivationAnimation();

    /// @brief Range from 0~1. 0 for no highlighting. 1 for full highlighting.
    qreal updateProgress;
    qreal getUpdateProgress() const;
    void setUpdateProgress(qreal newUpdateProgress);
    Q_PROPERTY(
        qreal updateProgress READ getUpdateProgress WRITE setUpdateProgress)

    QPropertyAnimation updateAnimation;
    /// @brief Restart the update animation
    /// @details The update animation includes changing the border highlighting
    void restartUpdateAnimation();
};

#endif // BUTTON_H
