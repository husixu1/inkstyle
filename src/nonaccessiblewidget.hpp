#ifndef NONACCESSIBLEWIDGET_HPP
#define NONACCESSIBLEWIDGET_HPP

#include <QAccessibleWidget>
#include <QWidget>

class NonAccessibleWidget : public QAccessibleWidget {
public:
    explicit NonAccessibleWidget(QWidget *w) : QAccessibleWidget(w) {}

    // relations
    int childCount() const override { return 0; }
    int indexOfChild(const QAccessibleInterface *) const override { return 0; }
    QAccessibleInterface *focusChild() const override { return nullptr; }

    // navigation
    QAccessibleInterface *parent() const override { return nullptr; }
    QAccessibleInterface *child(int) const override { return nullptr; }

    // properties and state
    QString text(QAccessible::Text) const override { return ""; }
    QAccessible::Role role() const override { return QAccessible::NoRole; }
    QAccessible::State state() const override { return QAccessible::State(); }
};

QAccessibleInterface *
nonAccessibleWidgetFactory(const QString &classname, QObject *object) {
    QAccessibleInterface *interface = 0;
    if (object && object->isWidgetType())
        interface = new NonAccessibleWidget(static_cast<QWidget *>(object));
    return interface;
}

#endif // NONACCESSIBLEWIDGET_HPP
