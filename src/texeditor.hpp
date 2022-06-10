#ifndef TEXEDITOR_HPP
#define TEXEDITOR_HPP

#include "configs.hpp"

#include <QObject>
#include <QProcess>
#include <QTemporaryFile>

class TexEditor : public QObject {
    Q_OBJECT
public:
    TexEditor(const QSharedPointer<Configs> &configs);
    void copyTextElement(const QByteArray &content);
    void copySvgElement(const QByteArray &content);

signals:
    /// @brief Emitted when the tex editor quits.
    /// @param content The context written in the editor
    void stopped(const QByteArray &content, bool compile);

public slots:
    /// @brief Start the tex editor in another process.
    /// @details When the editor stopped, #stopped will be triggered and the
    /// content in the editor will be passed as its argument;
    /// @param compile The #stopped singal will be emitted with the `compile`
    /// flag set the same as this parameter
    void start(bool compile);

private:
    /// @brief Start tex editor
    /// @return The temporary tex file to write, or null if editor not started.
    QSharedPointer<QTemporaryFile> startTexEditor();

    /// @brief compile tex file to pdf
    /// @return The compiled pdf file path, or "" if compilation failed.
    QString compileTexfile(const QSharedPointer<QTemporaryFile> &texFile);

    /// @brief convert pdf to svg
    /// @return The compiled pdf file path, or "" if compilation failed.
    QString convertPdfToSvg(const QString &pdfFile);

private:
    QSharedPointer<Configs> configs;
    QProcess editorProcess;
};

#endif // TEXEDITOR_HPP
