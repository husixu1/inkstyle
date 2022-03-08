#include "hiddenbutton.hpp"

#include <QPainter>

HiddenButton::HiddenButton(QWidget *parent) : QPushButton(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
}

void HiddenButton::enterEvent(QEvent *) {
    emit mouseEnter();
}

void HiddenButton::leaveEvent(QEvent *) {
    emit mouseLeave();
}

void HiddenButton::paintEvent(QPaintEvent *e) {
#if _WIN32
    // Windows requires a non-transparent area to capture the mouse pointer
    QPainter painter(this);
    painter.fillRect(rect(), QColor(255, 255, 255, 1));
#else
    QPushButton::paintEvent(e);
#endif
}
