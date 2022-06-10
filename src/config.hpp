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
#include <functional>
#include <yaml-cpp/yaml.h>

class CustomButtonInfo;
class StandardButtonInfo;

class Config : public QObject {
    Q_OBJECT

public:
    typedef quint32 Slot;
    typedef QMap<QString, QString> StylesList;

    bool hasButton(const Slot &slot) const;
    bool hasCustomButton(const Slot &slot) const;
    bool hasStandardButton(const Slot &slot) const;
    CustomButtonInfo getCustomButton(const Slot &slot) const;
    StandardButtonInfo getStandardButton(const Slot &slot) const;

    /// @brief Read config from file
    explicit Config(const QString &file = "", QObject *parent = nullptr);

    const QHash<QString, QString> &getSvgDefs() const;

    void updateStyle(
        const Slot &slot, const QHash<QString, QString> &styles,
        const QHash<QString, QString> &svgDefs = {});
    void saveToFile(const QString &file);

    QColor buttonBgColorInactive;
    QColor buttonBgColorActive;
    QColor guideColor;
    quint8 panelMaxLevels;
    quint32 panelRadius;
    QString defaultIconStyle;
    QString defaultIconText;
    QStringList texEditor;

private:
    /// @brief A list of buttons
    QHash<Slot, CustomButtonInfo> customButtons;
    QHash<Slot, StandardButtonInfo> standardButtons;

    /// @brief A list of svg defs (e.g. gradient, pattern, marker)
    /// @details stores: {defId, def-content}...
    QHash<QString, QString> svgDefs;

    /// @brief Parse global configs, initialize #guideColor, #panelRadius...
    /// @param config The root yaml node
    void parseGlobalConfig(const YAML::Node &config);

    /// @brief Parse global configs, initialize #svgDefs
    /// @param config The root yaml node
    void parseSvgDefsConfig(const YAML::Node &config);

    /// @brief Initialize #customButtons and #standardButtons
    /// @param config The root yaml node
    void parseButtonsConfig(const YAML::Node &config);
};

#endif // CONFIGMANAGER_HPP
