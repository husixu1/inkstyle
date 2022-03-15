#include "config.hpp"

#include "constants.hpp"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QVector>

namespace YAML {
// Allow direct conversion from yaml node to QString
template <>
struct convert<QString> {
    static Node encode(const QString &rhs) { return Node(rhs.toStdString()); }
    static bool decode(const Node &node, QString &rhs) {
        if (!node.IsScalar())
            return false;
        rhs = QString::fromStdString(node.Scalar());
        return true;
    }
};
template <>
struct convert<QByteArray> {
    static Node encode(const QByteArray &rhs) {
        return Node(rhs.toStdString());
    }
    static bool decode(const Node &node, QByteArray &rhs) {
        if (!node.IsScalar())
            return false;
        rhs = QByteArray::fromStdString(node.Scalar());
        return true;
    }
};
template <>
struct convert<QColor> {
    static Node encode(const QColor &rhs) { return Node(rhs.name()); }
    static bool decode(const Node &node, QColor &rhs) {
        if (!node.IsScalar())
            return false;
        rhs = QColor(node.Scalar().c_str());
        return true;
    }
};
} // namespace YAML

void Config::parseConfig(const YAML::Node &config) {
    parseGlobalConfig(config);
    parseButtonsConfig(config);
}

void Config::parseGlobalConfig(const YAML::Node &config) {
    namespace CC = C::C;
    namespace GK = C::C::G::K;
    namespace DIS = C::C::G::V::DIS;

    if (config[CC::global].IsDefined()) {
        if (!config[CC::global].IsMap())
            qWarning(R"("%s" is not a map, skipping...)", CC::global);
        const YAML::Node &gConfig = config[CC::global];

        // Check whether config exists and load it if exist
        auto loadGlobalConfig = [&]<typename T>(const char *key, T &config) {
            if (gConfig[key].IsDefined())
                config = gConfig[key].as<T>();
        };
        loadGlobalConfig(GK::panelBgColor, panelBgColor);
        loadGlobalConfig(GK::buttonBgColorInactive, buttonBgColorInactive);
        loadGlobalConfig(GK::buttonBgColorActive, buttonBgColorActive);
        loadGlobalConfig(GK::guideColor, guideColor);
        loadGlobalConfig(GK::panelMaxLevels, panelMaxLevels);
        loadGlobalConfig(GK::panelRadius, panelRadius);
        loadGlobalConfig(GK::defaultIconStyle, defaultIconStyle);
        loadGlobalConfig(GK::defaultIconText, defaultIconText);

        // Check sanity of global config
        if (!QSet<QString>({DIS::circle, DIS::square})
                 .contains(defaultIconStyle)) {
            qWarning(
                R"(%s:%s = "%s" is not recognized. Falling back to "%s")",
                CC::global, GK::defaultIconStyle,
                defaultIconStyle.toStdString().c_str(), DIS::circle);
            defaultIconStyle = DIS::circle;
        }
    }
}

void Config::parseButtonsConfig(const YAML::Node &config) {
    namespace CC = C::C;
    namespace BK = C::C::B::K;

    if (!config[CC::buttons].IsDefined())
        return;

    if (!config[CC::buttons].IsSequence())
        qWarning(R"("%s" is not a list, skipping...)", CC::buttons);

    // Valid config keys that a button can have
    QSet<QString> validKeys({BK::slot, BK::customIcon, BK::customStyle});
    for (const char *key : BK::basicStyles)
        validKeys.insert(key);

    size_t numButtons = 0;
    for (const YAML::Node &button : config[CC::buttons]) {
        ++numButtons;

        // Check for invalid keys
        for (const auto &elem : button)
            if (!validKeys.contains(elem.first.Scalar().c_str()))
                qWarning(
                    "Invalid key %s[%ld]:%s ignored.", CC::buttons, numButtons,
                    elem.first.Scalar().c_str());

        if (!button.IsMap())
            qWarning(
                "Button %s[%ld] is not a map, skipping...", CC::buttons,
                numButtons);

        // Check for validity and availability of slots
        Slot slot = button[BK::slot].as<Slot>();
        if (((slot >> 24) & 0xff) > panelMaxLevels * 6
            || ((slot >> 16) & 0xff) > 5 || ((slot >> 8) & 0xff) > 2
            || (slot & 0xff) > ((slot >> 8) & 0xff) * 2) {
            qWarning(
                "Slot %s[%ld]:%s = %#x invalid, skipping...", CC::buttons,
                numButtons, BK::slot, slot);
            continue;
        }
        if (buttons.contains(slot)) {
            qWarning(
                "Slot %s[%ld]:%s = %#x already registered, skipping...",
                CC::buttons, numButtons, BK::slot, slot);
            continue;
        }

        // Load button styles
        qDebug("Registering slot %#x", slot);

        QMap<QString, QString> styles;
        QStringList styleList;
        QByteArray styleSvg;

        if (button[BK::customStyle].IsDefined()) {
            // Load non-standard styles
            styleSvg = button[BK::customStyle].as<QString>().toUtf8();
        } else {
            // Load standard styles
            for (const char *style : BK::basicStyles)
                if (button[style].IsDefined()) {
                    styles.insert(QString(style), button[style].as<QString>());
                    styleList.append(
                        QString(style) + ":" + button[style].as<QString>());
                }

            // Compose styles to svg
            styleSvg = QString(R"(<?xml version="1.0" encoding="UTF-8"?>)"
                               R"(<svg><inkscape:clipboard style="%1"/></svg>)")
                           .arg(styleList.join(";"))
                           .toUtf8();
        }

        QByteArray icon;
        if (button[BK::customIcon].IsDefined())
            icon = button[icon].as<QString>().toUtf8();

        buttons.insert(
            slot, QSharedPointer<ButtonInfo>(
                      new ButtonInfo{*this, styleSvg, icon, styles}));
    }
}

Config::Config(const QString &file, QObject *parent) : QObject(parent) {

    // Parse default config
    qDebug("Loading default config.");
    QFile baseConfig(":/res/default.yaml");
    baseConfig.open(QFile::ReadOnly);
    YAML::Node defaultConfig = YAML::Load(baseConfig.readAll().toStdString());
    baseConfig.close();
    parseConfig(defaultConfig);

    // Parse user config
    if (QFile(file).exists()) {
        YAML::Node userConfig = YAML::LoadFile(file.toStdString());
        parseConfig(userConfig);
    } else {
        qWarning("No config file found. Using default config.");
    }
}

Config::Slot
Config::calcSlot(quint8 pSlot, quint8 tSlot, quint8 rSlot, quint8 subSlot) {
    return quint32(pSlot) << 24 | quint32(tSlot) << 16 | quint32(rSlot) << 8
           | quint32(subSlot);
}

QByteArray Config::ButtonInfo::genHash() const {
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(QString::number(reinterpret_cast<intptr_t>(&config)).toUtf8());
    hash.addData(styleSvg);
    hash.addData(userIconSvg);
    hash.addData(styles.keys().join("").toUtf8());
    hash.addData(styles.values().join("").toUtf8());
    return hash.result();
}

bool Config::ButtonInfo::validateHash(const QByteArray &hash) const {
    return genHash() == hash;
}
