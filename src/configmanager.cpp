#include "configmanager.hpp"

#include "constants.hpp"

#include <QDebug>
#include <QFile>
#include <QStringList>

void ConfigManager::parseConfig(const YAML::Node &config) {
    using namespace C::CK;

    if (config[globalConfig].IsDefined()) {
        if (!config[globalConfig].IsMap())
            qWarning() << "'" << globalConfig << "' is not a map, skipping";
        const YAML::Node &gConfig = config[globalConfig];

        // load global configs
        if (gConfig[GK::panelBgColor].IsScalar())
            panelBgColor = gConfig[GK::panelBgColor].as<quint32>();
        if (gConfig[GK::buttonBgColor].IsScalar())
            panelBgColor = gConfig[GK::buttonBgColor].as<quint32>();
        if (gConfig[GK::guideColor].IsScalar())
            panelBgColor = gConfig[GK::guideColor].as<quint32>();
        if (gConfig[GK::panelMaxLevels].IsScalar())
            panelMaxLevels = gConfig[GK::panelMaxLevels].as<quint8>();
        if (gConfig[GK::panelRadius].IsScalar())
            panelRadius = gConfig[GK::panelRadius].as<quint32>();
    }

    if (config[buttonsConfig].IsDefined()) {
        if (!config[buttonsConfig].IsSequence())
            qWarning() << "'" << buttonsConfig
                       << "' is not a list. Skipping...";

        size_t numButtons = 0;
        for (const YAML::Node &button : config[buttonsConfig]) {
            if (!button.IsMap())
                qWarning() << "The " << (numButtons + 1)
                           << "-th button is not defined as a map. Skipping...";

            // Check for validity and availability of slots
            if (!button[BK::slot].IsScalar()) {
                qWarning() << "The " << (numButtons + 1)
                           << "-th button's \"slot\" value is either not "
                              "defined or not a number. Skipping...";
                continue;
            }
            Slot slot = button[BK::slot].as<Slot>();
            if (((slot >> 24) & 0xff) > panelMaxLevels * 6
                || ((slot >> 16) & 0xff) > 5 || ((slot >> 8) & 0xff) > 2
                || (slot & 0xff) > ((slot >> 8) & 0xff) * 2) {
                qWarning() << "Slot " << Qt::hex << slot
                           << " invalid. Skipping...";
                continue;
            }
            if (buttons.contains(slot)) {
                qWarning() << "Slot " << Qt::hex << slot
                           << " already registered. Skipping...";
                continue;
            }

            // Load button styles
            qDebug() << "Registering slot " << Qt::hex << slot;
            QStringList styles;

            // load standard styles
            static const char *standardStyles[] = {
                BK::strokeColor, BK::strokeWidth, BK::strokeDashArray,
                BK::strokeStart, BK::strokeEnd,   BK::fill,
                BK::fillOpacity, BK::fillGradient};
            for (const char *style : standardStyles)
                if (button[style].IsDefined())
                    styles.append(
                        QString(style) + ":"
                        + button[style].as<std::string>().c_str());

            // TODO: load non-standard styles

            // TODO: compose styles to xml
            buttons.insert(
                slot,
                ButtonInfo{
                    QIcon(/*TODO*/),
                    QString(C::SC::plainStyleTemplate)
                        .replace(C::SC::stylePlaceHolder, styles.join(";"))});
            ++numButtons;
        }
    }
}

ConfigManager::ConfigManager(const QString configFile, QObject *parent)
    : QObject(parent) {

    // Parse default config
    QFile baseConfig(":/res/default.yaml");
    baseConfig.open(QFile::ReadOnly);
    YAML::Node defaultConfig = YAML::Load(baseConfig.readAll().toStdString());
    baseConfig.close();
    parseConfig(defaultConfig);

    // Parse user config
    if (QFile(configFile).exists()) {
        YAML::Node userConfig = YAML::LoadFile(configFile.toStdString());
        parseConfig(userConfig);
    } else {
        qWarning() << "No config file found. Using default config.";
    }
}

ConfigManager::Slot ConfigManager::calcSlot(
    quint8 pSlot, quint8 tSlot, quint8 rSlot, quint8 subSlot) {
    return quint32(pSlot) << 24 | quint32(tSlot) << 16 | quint32(rSlot) << 8
           | quint32(subSlot);
}
