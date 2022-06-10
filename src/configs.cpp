#include "configs.hpp"

#include <algorithm>

Configs::Configs(
    const QString &userConfigPath, const QString &generatedConfigPath,
    QObject *parent)
    : QObject{parent},
      configs{
          QSharedPointer<Config>(new Config(generatedConfigPath)),
          QSharedPointer<Config>(new Config(userConfigPath)),
          QSharedPointer<Config>(new Config)},
      generatedConfig(configs[0].data()), userConfig(configs[1].data()),
      defaultConfig(configs[2].data()) {

    // compose entries from underlying configs
    auto loadEntry = [&, this]<typename T>(T &result, T Config::*entryPtr) {
        result = defaultConfig->*entryPtr;
        for (auto &c : configs) {
            if (c.data()->*entryPtr != defaultConfig->*entryPtr) {
                result = c.data()->*entryPtr;
                break;
            }
        }
    };

    loadEntry(panelBgColor, &Config::panelBgColor);
    loadEntry(buttonBgColorInactive, &Config::buttonBgColorInactive);
    loadEntry(buttonBgColorActive, &Config::buttonBgColorActive);
    loadEntry(guideColor, &Config::guideColor);
    loadEntry(panelMaxLevels, &Config::panelMaxLevels);
    loadEntry(panelRadius, &Config::panelRadius);
    loadEntry(defaultIconStyle, &Config::defaultIconStyle);
    loadEntry(defaultIconText, &Config::defaultIconText);
    loadEntry(texEditor, &Config::texEditor);
}

bool Configs::hasButton(const Slot &slot) const {
    return std::any_of(configs.begin(), configs.end(), [&](const auto &c) {
        return c->hasButton(slot);
    });
}

bool Configs::hasCustomButton(const Slot &slot) const {
    return std::any_of(configs.begin(), configs.end(), [&](const auto &c) {
        return c->hasCustomButton(slot);
    });
}

bool Configs::hasStandardButton(const Slot &slot) const {
    return std::any_of(configs.begin(), configs.end(), [&](const auto &c) {
        return c->hasStandardButton(slot);
    });
}

CustomButtonInfo Configs::getCustomButton(const Slot &slot) const {
    return (*std::find_if(
                configs.begin(), configs.end(),
                [&](const auto &c) { return c->hasCustomButton(slot); }))
        ->getCustomButton(slot);
}

StandardButtonInfo Configs::getStandardButton(const Slot &slot) const {
    return (*std::find_if(
                configs.begin(), configs.end(),
                [&](const auto &c) { return c->hasStandardButton(slot); }))
        ->getStandardButton(slot);
}

QHash<QString, QString> Configs::getSvgDefs() const {
    // Stack the svgDefs and return
    QHash<QString, QString> svgDefs;
    std::for_each(configs.crbegin(), configs.crend(), [&](const auto &c) {
        svgDefs.insert(c->getSvgDefs());
    });
    return svgDefs;
}
