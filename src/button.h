#ifndef BUTTON_H
#define BUTTON_H

#include <QPushButton>

class Button : public QPushButton {
    Q_OBJECT
public:
    double rotation;

    Button();
    void redraw();
};

#endif // BUTTON_H
