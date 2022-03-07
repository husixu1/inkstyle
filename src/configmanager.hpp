#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include <QColor>
#include <QHash>
#include <QIcon>
#include <QObject>
#include <QString>
#include <yaml-cpp/yaml.h>

class ConfigManager : public QObject {
    Q_OBJECT

private:
    void parseConfig(const YAML::Node &config);

public:
    explicit ConfigManager(const QString configFile, QObject *parent = nullptr);

    QColor panelBgColor;
    QColor buttonBgColor;
    QColor guideColor;
    quint8 panelMaxLevels;
    quint32 panelRadius;

    struct ButtonInfo {
        QIcon icon;
        QString styleSvg;
    };
    QHash<quint16, ButtonInfo> buttons;
};

#endif // CONFIGMANAGER_HPP
