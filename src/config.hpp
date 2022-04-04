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
        /// @brief Associated ConfigManager
        const Config &config;

        /// @brief This is a static svg for copy-pasting
        const QByteArray styleSvg;

        /// @brief The icon svg provided by the user
        const QByteArray userIconSvg;

        /// @brief A list of key-value pairs
        /// @details Key will be one of the constant in #C::CK::BK
        const QMap<QString, QString> styles;

        /// @brief Ids of the svg definitions used by this button
        const QSet<QString> defIds;

        /// @brief Generate hash of this object (for caching purposes)
        QByteArray genHash() const;
        /// @brief Validate hash of this object (for caching purposes)
        bool validateHash(const QByteArray &hash) const;
    };

    typedef quint32 Slot;

    static Slot
    calcSlot(quint8 pSlot, quint8 tSlot, quint8 rSlot, quint8 subSlot);

    /// @brief A list of buttons
    QHash<Slot, QSharedPointer<ButtonInfo>> buttons;
    /// @brief A list of svg defs (e.g. gradient, pattern, marker)
    QHash<QString, QString> svgDefs;

private:
    void parseConfig(const YAML::Node &config);
    void parseGlobalConfig(const YAML::Node &config);
    void parseSvgDefsConfig(const YAML::Node &config);
    void parseButtonsConfig(const YAML::Node &config);
};

#endif // CONFIGMANAGER_HPP
