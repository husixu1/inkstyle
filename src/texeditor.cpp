#include "texeditor.hpp"

#include "constants.hpp"
#include "global.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QMimeData>
#include <QRegularExpression>
#include <QTemporaryFile>

TexEditor::TexEditor(const QSharedPointer<Configs> &configs)
    : QObject(nullptr), configs(configs) {

    if (configs->texEditorCmd.size() == 0)
        qWarning("Tex editor command not set.");
    if (configs->texCompileCmd.size() == 0)
        qWarning("Tex compiler command not set.");
    if (configs->pdfToSvgCmd.size() == 0)
        qWarning("pdf to svg command not set.");

    // Create a subdirectory under /tmp for subsequent operations
    if (!QDir::temp().exists(QString(EXE_NAME_STR)))
        QDir::temp().mkdir(EXE_NAME_STR);
}

void TexEditor::start(bool compile) {
    QSharedPointer<QTemporaryFile> texFile = startTexEditor();

    QMetaObject::Connection *const connection = new QMetaObject::Connection;
    *connection = connect(
        &editorProcess,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
        [this, connection, compile, texFile](int, QProcess::ExitStatus) {
            // Compile document if necessary
            if (compile) {
                QString pdfFile = compileTexfile(texFile);
                if (!pdfFile.isEmpty()) {
                    QString svgFilePath = convertPdfToSvg(pdfFile);
                    if (!svgFilePath.isEmpty()) {
                        QFile svgFile(svgFilePath);
                        svgFile.open(QFile::ReadOnly);
                        emit stopped(svgFile.readAll(), compile);

                        // Cleanup generated svg file
                        svgFile.remove();
                    }

                    // Cleanup generated tex-related files
                    static const QRegularExpression S(R"(\.pdf$)");
                    for (const auto &s : {".pdf", ".aux", ".log"})
                        if (QFile f(QString(pdfFile).replace(S, s)); f.exists())
                            f.remove();
                }
            } else {
                emit stopped(texFile->readAll(), compile);
            }

            // Cleanup temporary file
            texFile->close();
            texFile->deleteLater();

            // This is a single-shot connection
            disconnect(*connection);
            delete connection;
        });
}

QSharedPointer<QTemporaryFile> TexEditor::startTexEditor() {
    const QStringList &cmd = configs->texEditorCmd;
    if (!cmd.size())
        return {};

    if (editorProcess.state() == QProcess::Running) {
        qWarning("Tex editor process already running");
        return {};
    }

    QString program(cmd[0]);
    QStringList args(cmd.begin() + 1, cmd.end());

    // Create temporary file for editing
    QSharedPointer<QTemporaryFile> texFile(
        new QTemporaryFile(QDir::temp().filePath(EXE_NAME_STR "/XXXXXX.tex")));
    texFile->open();
    for (QString &arg : args)
        arg.replace("{{FILE}}", texFile->fileName());

    // Start program
    editorProcess.start(program, args);
    return texFile;
}

QString
TexEditor::compileTexfile(const QSharedPointer<QTemporaryFile> &texFile) {
    const QStringList &cmd = configs->texCompileCmd;
    if (!cmd.size())
        return {};

    qDebug("Compiling %s", texFile->fileName().toStdString().c_str());
    QString program(cmd[0]);
    QStringList args(cmd.begin() + 1, cmd.end());

    // Read texFile content into template body, and compose a new tex file
    QString texTemplate = configs->texCompileTemplate;
    texTemplate.replace("{{CONTENT}}", texFile->readAll());
    QTemporaryFile composedTexFile(
        QDir::temp().filePath(EXE_NAME_STR "/XXXXXX.tex"));
    composedTexFile.setAutoRemove(true);
    composedTexFile.open();
    composedTexFile.write(texTemplate.toUtf8());
    composedTexFile.close();

    QProcess compileProcess;
    compileProcess.setWorkingDirectory(QDir::temp().path() + "/" EXE_NAME_STR);
    for (QString &arg : args)
        arg.replace(
            "{{FILE}}", QFileInfo(composedTexFile.fileName()).fileName());
    compileProcess.start(program, args);

    if (compileProcess.waitForFinished(30000)) {
        if (compileProcess.exitCode() == 0) {
            qDebug("Compilation finished");
            static const QRegularExpression texSuffix(R"(\.tex$)");
            return QString(composedTexFile.fileName())
                .replace(texSuffix, "")
                .append(".pdf");
        } else {
            qCritical() << compileProcess.readAllStandardError();
        }
    } else {
        // terminate if compile not finished in 30s
        qCritical("Compilation not finished in 30s. Force stopping...");
        compileProcess.terminate();
    }
    return {};
}

QString TexEditor::convertPdfToSvg(const QString &pdfFile) {
    const QStringList &cmd = configs->pdfToSvgCmd;
    if (!cmd.size())
        return {};

    qDebug("Converting %s", pdfFile.toStdString().c_str());
    static const QRegularExpression pdfSuffix(R"(\.pdf$)");
    QString svgFile = QString(pdfFile).replace(pdfSuffix, "").append(".svg");

    QProcess convertProcess;
    QString program(cmd[0]);
    QStringList args(cmd.begin() + 1, cmd.end());
    for (QString &arg : args) {
        arg.replace("{{FILE_IN}}", pdfFile);
        arg.replace("{{FILE_OUT}}", svgFile);
    }

    convertProcess.start(program, args);
    if (convertProcess.waitForFinished(5000)) {
        if (convertProcess.exitCode() == 0) {
            qDebug("Conversion finished");
            return QString(svgFile);
        } else
            qCritical() << convertProcess.readAllStandardError();
    } else {
        // terminate if compile not finished in 30s
        qCritical("pdf -> svg not finished in 5s. Force stopping...");
        convertProcess.terminate();
    }
    return {};
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

void TexEditor::copySvgElement(const QByteArray &content) {
    QMimeData *elemSvg = new QMimeData;
    elemSvg->setData(C::styleMimeType, content);
    QApplication::clipboard()->setMimeData(elemSvg);
    qDebug() << "Element copied " << elemSvg->data(C::styleMimeType);
}
