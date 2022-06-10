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

        // The value of texEditor is a string list
        if (gConfig[GK::texEditor].IsDefined()) {
            if (!gConfig[GK::texEditor].IsSequence())
                qWarning(
                    R"("%s:%s" is not a list, skipping...)", CC::global,
                    GK::texEditor);
            size_t cmdCount = 0;
            for (const YAML::Node &cmd : gConfig[GK::texEditor]) {
                if (!cmd.IsScalar()) {
                    qWarning(
                        R"(%s:%s[%ld] is not a string, skipping)", CC::global,
                        GK::texEditor, cmdCount);
                    texEditor.clear();
                }
                texEditor.append(cmd.as<QString>());
                ++cmdCount;
            }
        }

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
        Slot slot = button[BK::slot].as<Slot>();
        if (((slot >> 24) & 0xff) > panelMaxLevels * 6
            || ((slot >> 16) & 0xff) > 5 || ((slot >> 8) & 0xff) > 2
            || (slot & 0xff) > ((slot >> 8) & 0xff) * 2) {
            qWarning(
                "Button %s[%ld]:%s = %#x invalid, skipping...", CC::buttons,
                numButtons, BK::slot, slot);
            continue;
        }
        if (hasButton(slot)) {
            qWarning(
                "Button %s[%ld]:%s = %#x already registered, skipping...",
                CC::buttons, numButtons, BK::slot, slot);
            continue;
        }

        // Load button styles
        qDebug("Registering button %#x", slot);

        // A function to get a set of defs used in styles
        auto genDefIds = [&, this](const QString &style) -> QSet<QString> {
            // import elements in "<defs>...</defs>" when composing button
            // styles
            QSet<QString> defIds;
            static const QRegularExpression urlRegEx(
                R"-(\burl\((?<op>['"]?)#(?<id>.*?)\k<op>\))-");
            auto matchIter = urlRegEx.globalMatch(style);
            while (matchIter.hasNext()) {
                QRegularExpressionMatch match = matchIter.next();
                QString defId = match.captured("id");
                if (svgDefs.contains(defId) && !defIds.contains(defId)) {
                    defIds.insert(defId);
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
            return defIds;
        };

        if (button[BK::customStyle].IsDefined()) {
            // Load non-standard styles
            QString style = button[BK::customStyle].as<QString>().toUtf8();
            QSet<QString> defIds = genDefIds(style);
            QByteArray customIcon;
            if (button[BK::customIcon].IsDefined())
                customIcon = button[BK::customIcon].as<QString>().toUtf8();
            customButtons.insert(slot, {defIds, style.toUtf8(), customIcon});
        } else {
            // Load standard styles
            QMap<QString, QString> styles;
            for (const char *style : BK::basicStyles)
                if (button[style].IsDefined())
                    styles.insert(QString(style), button[style].as<QString>());
            QSet<QString> defIds =
                genDefIds(QStringList(styles.begin(), styles.end()).join(";"));
            standardButtons.insert(slot, {defIds, styles});
        }
    }
}

Config::Config(const QString &file, QObject *parent) : QObject(parent) {
    // Parse default config
    qDebug("Loading default config.");
    QFile baseConfig(":/res/default.yaml");
    baseConfig.open(QFile::ReadOnly);
    YAML::Node defaultConfig = YAML::Load(baseConfig.readAll().toStdString());
    baseConfig.close();

    parseGlobalConfig(defaultConfig);
    if (file.isEmpty()) {
        // Only load svgdefs and buttons for default config (empty filename)
        parseSvgDefsConfig(defaultConfig);
        parseButtonsConfig(defaultConfig);
    }

    // Parse user config
    if (QFile(file).exists()) {
        YAML::Node userConfig = YAML::LoadFile(file.toStdString());
        parseGlobalConfig(userConfig);
        parseSvgDefsConfig(userConfig);
        parseButtonsConfig(userConfig);
    } else {
        qWarning("Config file %s not found.", file.toStdString().c_str());
    }
}

const QHash<QString, QString> &Config::getSvgDefs() const {
    return svgDefs;
}

bool Config::hasButton(const Slot &slot) const {
    return hasCustomButton(slot) || hasStandardButton(slot);
}

bool Config::hasCustomButton(const Slot &slot) const {
    return customButtons.contains(slot);
}

bool Config::hasStandardButton(const Slot &slot) const {
    return standardButtons.contains(slot);
}

CustomButtonInfo Config::getCustomButton(const Slot &slot) const {
    return customButtons[slot];
}

StandardButtonInfo Config::getStandardButton(const Slot &slot) const {
    return standardButtons[slot];
}

void ButtonInfo::clear() {
    defIds.clear();
}

ButtonInfo::operator bool() const {
    return !isEmpty();
}

bool ButtonInfo::operator==(const ButtonInfo &other) const {
    return defIds == other.defIds;
}

ButtonInfo &ButtonInfo::operator+=(const ButtonInfo &other) {
    defIds.unite(other.defIds);
    // Overwrite user-defined styles
    return *this;
}

size_t ButtonInfo::hash(size_t seed) const {
    return qHash(defIds, seed);
}

ButtonInfo::ButtonInfo(const QSet<QString> &defIds) : defIds(defIds) {}

QString ButtonInfo::genDefsSvg(const QHash<QString, QString> &svgDefs) const {
    QString defs;
    for (const auto &id : defIds)
        if (svgDefs.contains(id))
            defs.append(svgDefs[id]);
    return defs;
}

void CustomButtonInfo::clear() {
    ButtonInfo::clear();
    customStyleSvg.clear();
    customIconSvg.clear();
}

bool CustomButtonInfo::isEmpty() const {
    return customStyleSvg.isEmpty();
}

bool CustomButtonInfo::operator==(const CustomButtonInfo &other) const {
    return this->ButtonInfo::operator==(other)
           && customStyleSvg == other.customStyleSvg
           && customIconSvg == other.customIconSvg;
}

CustomButtonInfo &CustomButtonInfo::operator+=(const CustomButtonInfo &other) {
    this->ButtonInfo::operator+=(other);
    customIconSvg = other.customIconSvg;
    customStyleSvg = other.customStyleSvg;
    return *this;
}

size_t CustomButtonInfo::hash(size_t seed) const {
    return this->ButtonInfo::hash(seed) ^ qHash(customStyleSvg, seed)
           ^ qHash(customIconSvg, seed);
}

QByteArray
CustomButtonInfo::genStyleSvg(const QHash<QString, QString> &svgDefs) const {
    namespace BK = C::C::B::K;
    return QString(R"(<?xml version="1.0" encoding="UTF-8"?>)"
                   R"(<svg><defs>%1</defs>%2</svg>)")
        .arg(genDefsSvg(svgDefs), customStyleSvg)
        .toUtf8();
}

CustomButtonInfo::CustomButtonInfo(
    const QSet<QString> &defIds, const QByteArray &customStyleSvg,
    const QByteArray &customIconSvg)
    : ButtonInfo(defIds), customStyleSvg(customStyleSvg),
      customIconSvg(customIconSvg) {}

const QByteArray &CustomButtonInfo::getIconSvg() const {
    return customIconSvg;
}

void CustomButtonInfo::accept(ButtonInfoVisitor &visitor) {
    visitor.visit(*this);
}

void StandardButtonInfo::clear() {
    ButtonInfo::clear();
    styleList.clear();
}

bool StandardButtonInfo::isEmpty() const {
    return styleList.isEmpty();
}

bool StandardButtonInfo::operator==(const StandardButtonInfo &other) const {
    return this->ButtonInfo::operator==(other) && styleList == other.styleList;
}

StandardButtonInfo &
StandardButtonInfo::operator+=(const StandardButtonInfo &other) {
    // Merge standard styles
    this->ButtonInfo::operator+=(other);
    styleList.insert(other.styleList);
    return *this;
}

size_t StandardButtonInfo::hash(size_t seed) const {
    return this->ButtonInfo::hash(seed) ^ qHash(styleList, seed);
}

QByteArray
StandardButtonInfo::genStyleSvg(const QHash<QString, QString> &svgDefs) const {
    namespace BK = C::C::B::K;

    // Generate style svg content
    QStringList styles;
    for (const char *key : BK::basicStyles)
        if (styleList.contains(key))
            styles.append(key + (":" + styleList[key]));

    // Combine them as style svg
    return QString(
               R"(<?xml version="1.0" encoding="UTF-8"?>)"
               R"(<svg><defs>%1</defs><inkscape:clipboard style="%2"/></svg>)")
        .arg(genDefsSvg(svgDefs), styles.join(";"))
        .toUtf8();
}

void StandardButtonInfo::accept(ButtonInfoVisitor &visitor) {
    visitor.visit(*this);
}

StandardButtonInfo::StandardButtonInfo(
    const QSet<QString> &defIds, const Config::StylesList &styles)
    : ButtonInfo(defIds), styleList(styles) {}

const Config::StylesList &StandardButtonInfo::styles() const {
    return styleList;
}

size_t qHash(const Config::StylesList &styles, size_t seed) {
    size_t hash = ~(size_t)0;
    for (auto itr = styles.begin(); itr != styles.end(); ++itr)
        hash ^= qHash(itr.key(), seed) ^ qHash(itr.value(), seed);
    return hash;
}

size_t qHash(const ButtonInfo &info, size_t seed) {
    return info.hash(seed);
}
