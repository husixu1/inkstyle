#include "configs.hpp"
#include "global.hpp"
#include "nonaccessiblewidget.hpp"
#include "panel.hpp"
#include "runguard.hpp"
#include "texeditor.hpp"
#include "utils.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QHotkey>
#include <QMenu>
#include <QMimeData>
#include <QProcess>
#include <QStandardPaths>
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
    QString configPath =
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (configPath.isEmpty()) {
        qCritical("Cannot access %s", configPath.toStdString().c_str());
        return 1;
    }
    if (!QDir(configPath).exists(EXE_NAME_STR))
        QDir(configPath).mkdir(EXE_NAME_STR);
    configPath += "/" EXE_NAME_STR;
    QSharedPointer<Configs> configs(new Configs(
        configPath + "/config.yaml", configPath + "/config.generated.yaml"));

    // Don't quit on last window closed
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    // Disable accessibility for all widgets to avoid crash under certain WMs
    QAccessible::installFactory(nonAccessibleWidgetFactory);

    // Set app style
    QFile file(":/res/default.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet(file.readAll());
    a.setStyleSheet(styleSheet);

    // Register hotkeys
    QSharedPointer<Panel> panel(nullptr);
    QSharedPointer<QHotkey> hotkey1;
    if (!configs->shortcutMainPanel.isEmpty()) {
        hotkey1 = QSharedPointer<QHotkey>(
            new QHotkey(QKeySequence(configs->shortcutMainPanel), true, &a));
        QObject::connect(hotkey1.data(), &QHotkey::activated, qApp, [&]() {
            qDebug() << "Hotkey Activated";
            if (!panel)
                panel = QSharedPointer<Panel>(new Panel(nullptr, 0, configs));
            panel->show();
        });
        QObject::connect(hotkey1.data(), &QHotkey::released, qApp, [&]() {
            qDebug() << "Hotkey Released";
            if (panel) {
                panel->copyStyle();
                panel->close();
            }
            panel = nullptr;
            Utils::pasteStyleToInkscape();
        });
    }

    TexEditor editor(configs);
    QSharedPointer<QHotkey> hotkey2;
    if (!configs->shortcutTex.isEmpty()) {
        hotkey2 = QSharedPointer<QHotkey>(
            new QHotkey(QKeySequence(configs->shortcutTex), true, &a));
        QObject::connect(hotkey2.data(), &QHotkey::activated, &editor, [&] {
            editor.start(false);
        });
        QObject::connect(
            &editor, &TexEditor::stopped,
            [&](const QByteArray &data, bool compile) {
                if (compile)
                    return;
                if (!data.trimmed().size()) {
                    qInfo("Content empty. Nothing copied");
                    return;
                }
                editor.copyTextElement(data.trimmed());
                Utils::pasteElementToInkscape();
            });
    }

    QSharedPointer<QHotkey> hotkey3;
    if (!configs->shortcutCompiledTex.isEmpty()) {
        hotkey3 = QSharedPointer<QHotkey>(
            new QHotkey(QKeySequence(configs->shortcutCompiledTex), true, &a));
        QObject::connect(hotkey3.data(), &QHotkey::activated, &editor, [&] {
            editor.start(true);
        });
        QObject::connect(
            &editor, &TexEditor::stopped,
            [&](const QByteArray &data, bool compile) {
                if (!compile)
                    return;
                if (!data.trimmed().size()) {
                    qInfo("Content empty. Nothing copied");
                    return;
                }
                editor.copySvgElement(data.trimmed());
                Utils::pasteElementToInkscape();
            });
    }

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
