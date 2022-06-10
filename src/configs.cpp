#include "configs.hpp"

#include "buttoninfo.hpp"

#include <algorithm>

Configs::Configs(
    const QString &userConfigPath, const QString &generatedConfigPath,
    QObject *parent)
    : QObject{parent},
      configs{
          QSharedPointer<Config>(new Config(generatedConfigPath)),
          QSharedPointer<Config>(new Config(userConfigPath)),
          QSharedPointer<Config>(new Config)},
      generatedConfig(*configs[0]), userConfig(*configs[1]),
      defaultConfig(*configs[2]), generatedConfigPath(generatedConfigPath) {

    // compose entries from underlying configs
    auto loadEntry = [&, this]<typename T>(T &result, T Config::*entryPtr) {
        result = defaultConfig.*entryPtr;
        for (auto &c : configs) {
            if (c.data()->*entryPtr != defaultConfig.*entryPtr) {
                result = c.data()->*entryPtr;
                break;
            }
        }
    };
    loadEntry(shortcutMainPanel, &Config::shortcutMainPanel);
    loadEntry(shortcutTex, &Config::shortcutTex);
    loadEntry(shortcutCompiledTex, &Config::shortcutCompiledTex);
    loadEntry(buttonBgColorInactive, &Config::buttonBgColorInactive);
    loadEntry(buttonBgColorActive, &Config::buttonBgColorActive);
    loadEntry(guideColor, &Config::guideColor);
    loadEntry(panelMaxLevels, &Config::panelMaxLevels);
    loadEntry(panelRadius, &Config::panelRadius);
    loadEntry(defaultIconStyle, &Config::defaultIconStyle);
    loadEntry(defaultIconText, &Config::defaultIconText);
    loadEntry(texCompileTemplate, &Config::texCompileTemplate);
    loadEntry(texEditorCmd, &Config::texEditorCmd);
    loadEntry(texCompileCmd, &Config::texCompileCmd);
    loadEntry(pdfToSvgCmd, &Config::pdfToSvgCmd);
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

void Configs::updateGeneratedConfig(
    const Slot &slot, const QHash<QString, QString> &styles,
    const QHash<QString, QString> &svgDefs) {
    generatedConfig.updateStyle(slot, styles, svgDefs);
}

void Configs::saveGeneratedConfig() {
    generatedConfig.saveToFile(generatedConfigPath);
}
