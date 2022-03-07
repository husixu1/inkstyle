#include "panel.hpp"

#include <QApplication>
#include <QFile>
#include <QHotkey>

int main(int argc, char *argv[]) {

    QApplication a(argc, argv);

    // Don't quit on last window closed;
    a.setQuitOnLastWindowClosed(false);

    // Set app style
    QFile file(":/res/default.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet(file.readAll());
    a.setStyleSheet(styleSheet);

    // Register hotkey
    QHotkey hotkey(QKeySequence("Ctrl+Shift+F"), true, &a);
    QSharedPointer<Panel> panel(nullptr);

    QObject::connect(&hotkey, &QHotkey::activated, qApp, [&]() {
        qDebug() << "Hotkey Activated";
        if (!panel)
            panel = QSharedPointer<Panel>(new Panel);
        panel->show();
    });

    QObject::connect(&hotkey, &QHotkey::released, qApp, [&]() {
        qDebug() << "Hotkey Released";
        if (panel)
            panel->close();
        panel = nullptr;
    });

    return a.exec();
}
