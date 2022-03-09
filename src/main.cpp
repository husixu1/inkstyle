#include "configmanager.hpp"
#include "panel.hpp"
#include "utils.hpp"

#include <QApplication>
#include <QFile>
#include <QHotkey>

int main(int argc, char *argv[]) try {
    // Read config
    ConfigManager config("res/inkstyle.yaml");
    Panel::setConfig(&config);

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
    QObject::connect(&hotkey, &QHotkey::activated, [&]() {
        qDebug() << "Hotkey Activated";
        if (!panel)
            panel = QSharedPointer<Panel>(new Panel);
        panel->show();
    });
    QObject::connect(&hotkey, &QHotkey::released, [&]() {
        qDebug() << "Hotkey Released";
        if (panel)
            panel->close();
        panel = nullptr;
        Utils::pasteToInkscape();
    });

    return a.exec();
} catch (std::exception &e) {
    qCritical() << e.what();
    return 0;
}
