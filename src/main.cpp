#include "global.hpp"
#include "panel.hpp"
#include "runguard.hpp"
#include "texeditor.hpp"
#include "utils.hpp"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QHotkey>
#include <QMenu>
#include <QMimeData>
#include <QProcess>
#include <QSystemTrayIcon>
#include <iostream>

int main(int argc, char *argv[]) try {
    // Run one instance only
    RunGuard guard("inkstyle");
    if (!guard.tryToRun()) {
        qCritical() << "Another instance of InkStyle is running.";
        return 0;
    }

    // Read config
    QSharedPointer<Config> config(new Config("res/inkstyle.yaml"));

    // Don't quit on last window closed
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    // Set app style
    QFile file(":/res/default.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet(file.readAll());
    a.setStyleSheet(styleSheet);

    // Register hotkeys
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
        Utils::pasteStyleToInkscape();
    });

    QHotkey hotkey_2(QKeySequence("Ctrl+T"), true, &a);
    TexEditor editor(config);
    QObject::connect(
        &hotkey_2, &QHotkey::activated, &editor, &TexEditor::start);
    QObject::connect(&editor, &TexEditor::stopped, [&](const QByteArray &data) {
        if (!data.trimmed().size()) {
            qInfo("Content empty. Nothing copied");
            return;
        }
        editor.copyTextElement(data.trimmed());
        Utils::pasteElementToInkscape();
    });

    // Create the tray icon
    QSystemTrayIcon trayIcon(QPixmap(":/res/icons/tray_icon.png"));
    QMenu trayMenu;
    trayMenu.addAction("Exit", qApp, &QApplication::quit);
    trayIcon.setContextMenu(&trayMenu);
    trayIcon.show();

    return a.exec();
} catch (std::exception &e) {
    qCritical() << e.what();
    return 0;
}
