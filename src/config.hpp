#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include <QColor>
#include <QHash>
#include <QIcon>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QVector>
#include <yaml-cpp/yaml.h>

class Config : public QObject {
    Q_OBJECT

public:
    typedef quint32 Slot;
    typedef QMap<QString, QString> StylesList;

    explicit Config(const QString &file, QObject *parent = nullptr);

    QColor panelBgColor;
    QColor buttonBgColorInactive;
    QColor buttonBgColorActive;
    QColor guideColor;
    quint8 panelMaxLevels;
    quint32 panelRadius;
    QString defaultIconStyle;
    QString defaultIconText;

    struct ButtonInfo {
        /// @brief This is a static svg for copy-pasting
        QByteArray styleSvg;

        /// @brief The icon svg provided by the user
        QByteArray userIconSvg;

        /// @brief A list of key-value pairs
        /// @details Key will be one of the constant in #C::CK::BK
        StylesList styles;

        /// @brief Ids of the svg definitions used by this button
        /// @see Config::svgDefs
        QSet<QString> defIds;

        bool operator==(const ButtonInfo &other) const;
    };

    static Slot
    calcSlot(quint8 pSlot, quint8 tSlot, quint8 rSlot, quint8 subSlot);

    /// @brief A list of buttons
    QHash<Slot, ButtonInfo> buttons;

    /// @brief A list of svg defs (e.g. gradient, pattern, marker)
    /// @details stores: {defId, def-content}...
    QHash<QString, QString> svgDefs;

private:
    void parseConfig(const YAML::Node &config);
    void parseGlobalConfig(const YAML::Node &config);
    void parseSvgDefsConfig(const YAML::Node &config);
    void parseButtonsConfig(const YAML::Node &config);
};

size_t qHash(const Config::StylesList &styles, size_t seed = 0);
size_t qHash(const Config::ButtonInfo &info, size_t seed = 0);

#endif // CONFIGMANAGER_HPP
