#ifndef TEXEDITOR_HPP
#define TEXEDITOR_HPP

#include "config.hpp"

#include <QObject>
#include <QProcess>
#include <QTemporaryFile>

class TexEditor : public QObject {
    Q_OBJECT
public:
    TexEditor(const QSharedPointer<Config> &config);
    void copyTextElement(const QByteArray &content);

signals:
    /// @brief Emitted when the tex editor quits.
    /// @param content The context written in the editor
    void stopped(const QByteArray &content);

public slots:
    /// @brief Start the tex editor in another process.
    /// @details When the editor stopped, #stopped will be triggered and the
    /// content in the editor will be passed as its argument;
    void start();

private:
    QSharedPointer<Config> config;
    QProcess editorProcess;
    QSharedPointer<QTemporaryFile> texFile;
};

#endif // TEXEDITOR_HPP
