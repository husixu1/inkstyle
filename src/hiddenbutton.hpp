#ifndef HIDDENBUTTON_H
#define HIDDENBUTTON_H

#include <QPushButton>

class HiddenButton : public QPushButton {
    Q_OBJECT
public:
    HiddenButton(QWidget *parent = nullptr);

protected:
    virtual void enterEvent(QEvent *e) override;
    virtual void leaveEvent(QEvent *e) override;

signals:
    void mouseEnter();
    void mouseLeave();
};

#endif // HIDDENBUTTON_H
