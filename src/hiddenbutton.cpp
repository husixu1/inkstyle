#include "hiddenbutton.h"

HiddenButton::HiddenButton(QWidget *parent) : QPushButton(parent) {}

void HiddenButton::enterEvent(QEvent *e) {
    emit mouseEnter();
}

void HiddenButton::leaveEvent(QEvent *e) {
    emit mouseLeave();
}
