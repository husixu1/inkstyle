#ifndef CONFIGS_HPP
#    define CONFIGS_HPP

#    include "config.hpp"

#    include <QObject>
#    include <QSharedPointer>
#    include <QVector>

class Configs : public QObject {
    Q_OBJECT
public:
    explicit Configs(
        const QString &userConfigPath, const QString &generatedConfigPath,
        QObject *parent = nullptr);

    using Slot = Config::Slot;

    bool hasButton(const Slot &slot) const;
    bool hasCustomButton(const Slot &slot) const;
    bool hasStandardButton(const Slot &slot) const;
    CustomButtonInfo getCustomButton(const Slot &slot) const;
    StandardButtonInfo getStandardButton(const Slot &slot) const;

    QHash<QString, QString> getSvgDefs() const;

    QColor panelBgColor;
    QColor buttonBgColorInactive;
    QColor buttonBgColorActive;
    QColor guideColor;
    quint8 panelMaxLevels;
    quint32 panelRadius;
    QString defaultIconStyle;
    QString defaultIconText;
    QStringList texEditor;

    // TODO: add interface to write to generatedConfig

private:
    /// @brief A list of configs to stack
    QVector<QSharedPointer<Config>> configs;

    /// @{
    /// @brief Reference to #configs. Precedence: generated > user > default
    Config *const generatedConfig;
    Config *const userConfig;
    Config *const defaultConfig;
    /// @}
};

#endif // CONFIGS_HPP

// TODO: replace Config with Configs
