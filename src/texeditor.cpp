#include "texeditor.hpp"

#include "constants.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QMetaObject>
#include <QMimeData>
#include <QTemporaryFile>

TexEditor::TexEditor(const QSharedPointer<Config> &config)
    : QObject(nullptr), config(config) {
    if (config->texEditor.size() == 0)
        qWarning("Tex editor command not set. It won't be invoked.");
}

void TexEditor::start() {
    // Set command for the process
    const QStringList &cmd = config->texEditor;
    if (!cmd.size())
        return;

    if (editorProcess.state() == QProcess::Running) {
        qWarning("Tex editor process already running");
        return;
    }

    QString program(cmd[0]);
    QStringList args(cmd.begin() + 1, cmd.end());

    // Create temporary file for editing
    texFile = QSharedPointer<QTemporaryFile>(
        new QTemporaryFile(QDir::temp().filePath("XXXXXX.tex")));
    texFile->open();
    for (QString &arg : args)
        arg.replace("{{FILE}}", texFile->fileName());

    // Start program
    editorProcess.start(program, args);
    QMetaObject::Connection *const connection = new QMetaObject::Connection;
    *connection = connect(
        &editorProcess,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
        [this, connection](int, QProcess::ExitStatus) {
            emit stopped(texFile->readAll());
            // Cleanup temporary file
            texFile->close();
            texFile = nullptr;
            // Single-shot connection
            disconnect(*connection);
            delete connection;
        });
}

void TexEditor::copyTextElement(const QByteArray &content) {
    // Copy style associated with slot to clipboard
    QMimeData *elemSvg = new QMimeData;
    elemSvg->setData(
        C::styleMimeType,
        QString(R"(<?xml version="1.0" encoding="UTF-8"?>)"
                R"(<svg><text><tspan>%1</tspan></text></svg>)")
            .arg(QString(content))
            .toUtf8());
    QApplication::clipboard()->setMimeData(elemSvg);
    qDebug() << "Element copied " << elemSvg->data(C::styleMimeType);
}
