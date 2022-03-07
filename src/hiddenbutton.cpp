#include "hiddenbutton.hpp"

HiddenButton::HiddenButton(QWidget *parent) : QPushButton(parent) {}

void HiddenButton::enterEvent(QEvent *) {
    emit mouseEnter();
}

void HiddenButton::leaveEvent(QEvent *) {
    emit mouseLeave();
}
