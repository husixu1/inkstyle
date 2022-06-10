#include "global.hpp"
#include "panel.hpp"
#include "utils.hpp"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QHotkey>
#include <QMimeData>
#include <iostream>

int main(int argc, char *argv[]) try {
    // Read config
    QSharedPointer<Config> config(new Config("res/inkstyle.yaml"));

    QApplication a(argc, argv);

    // Don't quit on last window closed
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
            panel = QSharedPointer<Panel>(new Panel(nullptr, 0, config));
        panel->show();
    });
    QObject::connect(&hotkey, &QHotkey::released, qApp, [&]() {
        qDebug() << "Hotkey Released";
        if (panel) {
            panel->copyStyle();
            panel->close();
        }
        panel = nullptr;
        Utils::pasteToInkscape();
    });

    QHotkey hotkey_2(QKeySequence("Ctrl+T"), true, &a);
    QObject::connect(&hotkey_2, &QHotkey::activated, [&]() {
        qDebug() << "Hotkey 2 Activated";
        std::cout << QApplication::clipboard()
                         ->mimeData()
                         ->data(C::styleMimeType)
                         .toStdString()
                  << std::endl;
    });
    QObject::connect(&hotkey_2, &QHotkey::released, [&]() {
        qDebug() << "Hotkey 2 Released";
        // qApp->quit();
    });

    return a.exec();
} catch (std::exception &e) {
    qCritical() << e.what();
    return 0;
}
