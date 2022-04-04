#include "config.hpp"

#include "constants.hpp"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
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
    parseSvgDefsConfig(config);
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

void Config::parseSvgDefsConfig(const YAML::Node &config) {
    namespace CC = C::C;
    namespace K = C::C::SD::K;

    if (!config[CC::svgDefs].IsDefined())
        return;
    if (!config[CC::svgDefs].IsSequence())
        qWarning(R"("%s is not a list, skipping...")", CC::svgDefs);

    size_t numDefs = 0;
    for (const YAML::Node &def : config[CC::svgDefs]) {
        ++numDefs;
        // Type check
        if (!def.IsMap()) {
            qWarning("%s[%ld] is not a map, skipping...", CC::svgDefs, numDefs);
            continue;
        }
        if (!def[K::id].IsDefined()) {
            qWarning(
                R"(%s[%ld] missing "%s", skipping...)", CC::svgDefs, numDefs,
                K::id);
            continue;
        }
        if (!def[K::type].IsDefined()) {
            qWarning(
                R"(%s[%ld] missing "%s", skipping...)", CC::svgDefs, numDefs,
                K::type);
            continue;
        }
        if (def[K::attrs].IsDefined() && !def[K::attrs].IsMap()) {
            qWarning(
                R"(%s[%ld]:%s is not a map, skipping...)", CC::svgDefs, numDefs,
                K::attrs);
            continue;
        }

        QString id = def[K::id].as<QString>();
        if (svgDefs.contains(id)) {
            qWarning(
                R"(%s[%ld]:%s = "%s" already registered, skipping...)",
                CC::svgDefs, numDefs, K::id, id.toStdString().c_str());
            continue;
        }

        QString type = def[K::type].as<QString>();
        QString content;
        QStringList attrs;
        if (def[K::svg].IsDefined())
            content = def[K::svg].as<QString>();
        if (def[K::attrs].IsDefined())
            for (const auto &attr : def[K::attrs])
                attrs.append(QString(R"(%1="%2")")
                                 .arg(
                                     attr.first.as<QString>(),
                                     attr.second.as<QString>()));

        QString svgDef = QString(R"(<%1 id="%2" %3>%4</%1>)")
                             .arg(type, id, attrs.join(' '), content);
        svgDefs.insert(id, svgDef);
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
        // TODO: use a function to parse p/t/r/sub-slot
        Slot slot = button[BK::slot].as<Slot>();
        if (((slot >> 24) & 0xff) > panelMaxLevels * 6
            || ((slot >> 16) & 0xff) > 5 || ((slot >> 8) & 0xff) > 2
            || (slot & 0xff) > ((slot >> 8) & 0xff) * 2) {
            qWarning(
                "Button %s[%ld]:%s = %#x invalid, skipping...", CC::buttons,
                numButtons, BK::slot, slot);
            continue;
        }
        if (buttons.contains(slot)) {
            qWarning(
                "Button %s[%ld]:%s = %#x already registered, skipping...",
                CC::buttons, numButtons, BK::slot, slot);
            continue;
        }

        // Load button styles
        qDebug("Registering button %#x", slot);

        QMap<QString, QString> styles;
        QString styleElement;

        if (button[BK::customStyle].IsDefined()) {
            // Load non-standard styles
            styleElement = button[BK::customStyle].as<QString>().toUtf8();
        } else {
            // Load standard styles
            QStringList styleList;
            for (const char *style : BK::basicStyles)
                if (button[style].IsDefined()) {
                    styles.insert(QString(style), button[style].as<QString>());
                    styleList.append(
                        QString(style) + ":" + button[style].as<QString>());
                }

            // Compose styles to svg
            styleElement = QString(R"(<inkscape:clipboard style="%1"/>)")
                               .arg(styleList.join(";"));
        }

        // import elements in "<defs>...</defs>" when composing button styles
        QSet<QString> defIds;
        QString defs;
        static const QRegularExpression urlRegEx(
            R"-(\burl\((?<op>['"]?)#(?<id>.*?)\k<op>\))-");
        auto matchIter = urlRegEx.globalMatch(styleElement);
        while (matchIter.hasNext()) {
            QRegularExpressionMatch match = matchIter.next();
            QString defId = match.captured("id");
            if (svgDefs.contains(defId) && !defIds.contains(defId)) {
                defIds.insert(defId);
                defs.append(svgDefs[defId]);
                qDebug(
                    R"(Using svg def id="%s" for button %#x)",
                    defId.toStdString().c_str(), slot);
            } else if (!svgDefs.contains(defId)) {
                qWarning(
                    R"(Button %s[%ld]:%s = %#x using a def id="%s" which )"
                    R"(is not defined. Skipping importing this def...)",
                    CC::buttons, numButtons, BK::slot, slot,
                    defId.toStdString().c_str());
            }
        }

        QByteArray styleSvg =
            QString(R"(<?xml version="1.0" encoding="UTF-8"?>)"
                    R"(<svg><defs>%1</defs>%2</svg>)")
                .arg(defs, styleElement)
                .toUtf8();

        QByteArray icon;
        if (button[BK::customIcon].IsDefined())
            icon = button[icon].as<QString>().toUtf8();

        buttons.insert(slot, {styleSvg, icon, styles, defIds});
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
bool Config::ButtonInfo::operator==(const Config::ButtonInfo &other) const {
    return styleSvg == other.styleSvg && userIconSvg == other.userIconSvg
           && styles == other.styles && defIds == other.defIds;
}

void Config::ButtonInfo::clear() {
    styleSvg.clear();
    userIconSvg.clear();
    styles.clear();
    defIds.clear();
}

size_t qHash(const Config::StylesList &styles, size_t seed) {
    size_t hash = ~(size_t)0;
    for (auto itr = styles.begin(); itr != styles.end(); ++itr)
        hash ^= qHash(itr.key(), seed) ^ qHash(itr.value(), seed);
    return hash;
}

size_t qHash(const Config::ButtonInfo &info, size_t seed) {
    return qHash(info.styleSvg, seed) ^ qHash(info.userIconSvg, seed)
           ^ qHash(info.styles, seed) ^ qHash(info.defIds);
}
