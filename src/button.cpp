#include "button.h"

#include <iostream>

void Button::redraw() {}

void Button::enterEvent(QEvent *e) {
    std::cout << this << "mouse enter" << std::endl;
    emit mouseEnter();
}

void Button::leaveEvent(QEvent *e) {
    std::cout << this << "mouse leave" << std::endl;
    emit mouseLeave();
}
