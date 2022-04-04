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

    typedef quint32 Slot;

    static Slot
    calcSlot(quint8 pSlot, quint8 tSlot, quint8 rSlot, quint8 subSlot);

    /// @brief A list of buttons
    QHash<Slot, ButtonInfo> buttons;
};

#endif // CONFIGMANAGER_HPP
