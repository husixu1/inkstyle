#ifndef BUTTON_H
#define BUTTON_H

#include <QEvent>
#include <QPushButton>

class Button : public QPushButton {
    Q_OBJECT
public:
    using QPushButton::QPushButton;

    double rotation;
    void redraw();

    virtual void enterEvent(QEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;
signals:
    void mouseEnter();
    void mouseLeave();
};

#endif // BUTTON_H
