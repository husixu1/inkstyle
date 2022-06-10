#ifndef CONFIGS_HPP
#define CONFIGS_HPP

#include "config.hpp"

#include <QObject>
#include <QSharedPointer>
#include <QVector>

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

    /// @brief Update Generated Config
    void updateGeneratedConfig(
        const Slot &slot, const QHash<QString, QString> &styles,
        const QHash<QString, QString> &svgDefs = {});

    /// @brief Save the #generatedConfig to #generatedConfigPath
    void saveGeneratedConfig();

private:
    /// @brief A list of configs to stack
    QVector<QSharedPointer<Config>> configs;

    /// @{
    /// @brief Reference to #configs. Precedence: generated > user > default
    Config &generatedConfig;
    Config &userConfig;
    Config &defaultConfig;
    /// @}

    const QString generatedConfigPath;
};

#endif // CONFIGS_HPP
