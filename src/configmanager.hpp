#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include <QColor>
#include <QHash>
#include <QIcon>
#include <QObject>
#include <QString>

class ConfigManager : public QObject {
    Q_OBJECT
public:
    explicit ConfigManager(const QString configFile, QObject *parent = nullptr);

    QColor panelBgColor;
    QColor buttonBgColor;
    QColor guideColor;
    quint8 panelMaxLevels;
    quint32 panelRadius;

    class ButtonInfo {
        QIcon icon;
        QString styleSvg;
    };
    QHash<quint16, ButtonInfo> buttons;
};

#endif // CONFIGMANAGER_HPP
