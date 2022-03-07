#ifndef BUTTON_H
#define BUTTON_H

#include <QEvent>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRegion>

class Button : public QPushButton {
    Q_OBJECT
public:
    Button(QRect geometry, QRegion mask, QWidget *parent = nullptr);

protected:
    virtual void enterEvent(QEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;

    /// @brief Overriden to set mask on every resize
    virtual void resizeEvent(QResizeEvent *e) override;

    virtual void paintEvent(QPaintEvent *e) override;

private:
    QRect inactiveGeometry;
    QRegion inactiveMask;

    QPoint mousePos;
    bool hovering;
    const qreal hoverScale;

    QPropertyAnimation animation;
    void startAnimation();

signals:
    void mouseEnter();
    void mouseLeave();
};

#endif // BUTTON_H
