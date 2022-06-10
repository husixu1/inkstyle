#include "config.hpp"

#include "buttoninfo.hpp"
#include "constants.hpp"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QQueue>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVector>
#include <pugixml.hpp>

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
        loadGlobalConfig(GK::shortcutMainPanel, shortcutMainPanel);
        loadGlobalConfig(GK::shortcutTex, shortcutTex);
        loadGlobalConfig(GK::shortcutCompiledTex, shortcutCompiledTex);
        loadGlobalConfig(GK::buttonBgColorInactive, buttonBgColorInactive);
        loadGlobalConfig(GK::buttonBgColorActive, buttonBgColorActive);
        loadGlobalConfig(GK::guideColor, guideColor);
        loadGlobalConfig(GK::panelMaxLevels, panelMaxLevels);
        loadGlobalConfig(GK::panelRadius, panelRadius);
        loadGlobalConfig(GK::defaultIconStyle, defaultIconStyle);
        loadGlobalConfig(GK::defaultIconText, defaultIconText);
        loadGlobalConfig(GK::texCompileTemplate, texCompileTemplate);

        auto loadStringList = [&](const char *key, QStringList &config) {
            if (!gConfig[key].IsDefined())
                return;
            config.clear();
            if (gConfig[key].IsSequence()) {
                size_t cmdCount = 0;
                for (const YAML::Node &cmd : gConfig[key]) {
                    if (!cmd.IsScalar()) {
                        qWarning(
                            R"(%s:%s[%ld] is not a string, skipping...)",
                            CC::global, key, cmdCount);
                        config.clear();
                        break;
                    }
                    config.append(cmd.as<QString>());
                    ++cmdCount;
                }
            } else {
                qWarning(
                    R"("%s:%s" is not a list, skipping...)", CC::global, key);
            }
        };
        // Load string list configs
        loadStringList(GK::texEditorCmd, texEditorCmd);
        loadStringList(GK::texCompileCmd, texCompileCmd);
        loadStringList(GK::pdfToSvgCmd, pdfToSvgCmd);

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

        // Get custom icon if available
        QByteArray customIcon;
        if (button[BK::customIcon].IsDefined())
            customIcon = button[BK::customIcon].as<QString>().toUtf8();

        if (button[BK::customStyle].IsDefined()) {
            // Load non-standard styles
            QString style = button[BK::customStyle].as<QString>().toUtf8();
            QSet<QString> defIds = genDefIds(style);
            customButtons.insert(slot, {defIds, style.toUtf8(), customIcon});
        } else {
            // Load standard styles
            QMap<QString, QString> styles;
            for (const char *style : BK::basicStyles)
                if (button[style].IsDefined())
                    styles.insert(QString(style), button[style].as<QString>());
            QSet<QString> defIds =
                genDefIds(QStringList(styles.begin(), styles.end()).join(";"));
            standardButtons.insert(slot, {defIds, styles, customIcon});
        }
    }
}

Config::Config(const QString &file, QObject *parent) : QObject(parent) {
    // Parse default config
    qDebug("%s: Loading default config.", file.toStdString().c_str());
    QFile baseConfig(":/res/default.yaml");
    baseConfig.open(QFile::ReadOnly);
    YAML::Node defaultConfig = YAML::Load(baseConfig.readAll().toStdString());
    baseConfig.close();

    parseGlobalConfig(defaultConfig);
    if (file.isEmpty()) {
        // Only load svgdefs and buttons for default config (empty filename)
        parseSvgDefsConfig(defaultConfig);
        parseButtonsConfig(defaultConfig);
        return;
    }

    // Parse user config
    qDebug("%s: Loading user config.", file.toStdString().c_str());
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

void Config::updateStyle(
    const Slot &slot, const QHash<QString, QString> &styles,
    const QHash<QString, QString> &svgDefs) {

    namespace CC = C::C;
    namespace BK = C::C::B::K;

    for (const QString &key : svgDefs.keys())
        if (!this->svgDefs.contains(key))
            this->svgDefs.insert(key, svgDefs[key]);

    // A function to get a set of defs used in styles
    auto genDefIds = [&](const QString &style) -> QSet<QString> {
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
                    R"(Button %s:%s = %#x using a def id="%s" which )"
                    R"(is not defined. Skipping importing this def...)",
                    CC::buttons, BK::slot, slot, defId.toStdString().c_str());
            }
        }
        return defIds;
    };

    // Load standard styles
    QMap<QString, QString> stylesToSave;
    for (const char *key : BK::basicStyles)
        if (styles.contains(key))
            stylesToSave.insert(QString(key), styles[key]);
    QSet<QString> defIds =
        genDefIds(QStringList(styles.begin(), styles.end()).join(";"));
    standardButtons.insert(slot, {defIds, stylesToSave, {}});
}

void Config::saveToFile(const QString &file) {
    namespace CC = C::C;
    namespace GK = C::C::G::K;
    namespace BK = C::C::B::K;
    namespace SDK = C::C::SD::K;

    using QColor::HexArgb;
    using YAML::BeginDoc, YAML::EndDoc, YAML::BeginMap, YAML::EndMap,
        YAML::BeginSeq, YAML::EndSeq, YAML::Key, YAML::Value, YAML::Hex;

    // Save to file
    YAML::Emitter out;

    // Begin document
    out << BeginDoc << BeginMap;

    // Write global section
    {
        out << Key << CC::global << Value << BeginMap;

        out << Key << GK::buttonBgColorInactive << Value
            << buttonBgColorInactive.name(HexArgb).toStdString().c_str();
        out << Key << GK::buttonBgColorActive << Value
            << buttonBgColorActive.name(HexArgb).toStdString().c_str();
        out << Key << GK::guideColor << Value
            << guideColor.name(HexArgb).toStdString().c_str();
        out << Key << GK::panelMaxLevels << Value << unsigned(panelMaxLevels);
        out << Key << GK::panelRadius << Value << panelRadius;
        out << Key << GK::defaultIconStyle << Value
            << defaultIconStyle.toStdString().c_str();
        out << Key << GK::defaultIconText << Value
            << defaultIconText.toStdString().c_str();
        out << Key << GK::texCompileTemplate << Value
            << texCompileTemplate.toStdString().c_str();
        out << Key << GK::texEditorCmd << Value << BeginSeq;
        for (const QString &cmd : texEditorCmd)
            out << cmd.toStdString().c_str();
        out << EndSeq;
        out << Key << GK::texCompileCmd << Value << BeginSeq;
        for (const QString &cmd : texCompileCmd)
            out << cmd.toStdString().c_str();
        out << EndSeq;
        out << Key << GK::pdfToSvgCmd << Value << BeginSeq;
        for (const QString &cmd : pdfToSvgCmd)
            out << cmd.toStdString().c_str();
        out << EndSeq;

        out << EndMap;
    }

    // Write buttons section
    {
        out << Key << CC::buttons << Value << BeginSeq;
        for (auto itr = standardButtons.constKeyValueBegin();
             itr != standardButtons.constKeyValueEnd(); ++itr) {
            out << BeginMap << Key << BK::slot << Value << Hex << itr->first;
            const StylesList &styleList = itr->second.styles();
            for (auto styleItr = styleList.constKeyValueBegin();
                 styleItr != styleList.constKeyValueEnd(); ++styleItr)
                out << Key << styleItr->first.toStdString().c_str() << Value
                    << styleItr->second.toStdString().c_str();
            out << EndMap;
        }
        for (auto itr = customButtons.keyValueBegin();
             itr != customButtons.keyValueEnd(); ++itr) {
            out << BeginMap << Key << BK::slot << Value << itr->first;
            if (QByteArray style = itr->second.getStyleSvg(); !style.isEmpty())
                out << Key << BK::customStyle << Value
                    << style.toStdString().c_str();
            if (QByteArray icon = itr->second.getIconSvg(); !icon.isEmpty())
                out << Key << BK::customIcon << Value
                    << icon.toStdString().c_str();
            out << EndMap;
        }
        out << EndSeq;
    }

    // Write svgDefs section
    {
        out << Key << CC::svgDefs << Value << BeginSeq;
        for (auto itr = svgDefs.keyValueBegin(); itr != svgDefs.keyValueEnd();
             ++itr) {
            out << BeginMap << Key << SDK::id << Value
                << itr->first.toStdString().c_str();

            // Parse def attributes and child nodes
            pugi::xml_document doc;
            doc.load_string(itr->second.toStdString().c_str());
            pugi::xml_node def = doc.first_child();

            // Write type
            out << Key << SDK::type << Value << def.name();

            // Write attributes
            if (!def.attributes().empty()) {
                out << Key << SDK::attrs << Value << BeginMap;
                for (pugi::xml_attribute &attribute : def.attributes())
                    if (std::string(attribute.name()) != SDK::id)
                        out << Key << attribute.name() << Value
                            << attribute.value();
                out << EndMap;
            }

            // Write child nodes
            if (!def.children().empty()) {
                std::stringstream ss;
                for (pugi::xml_node &node : def.children())
                    node.print(ss);
                out << Key << SDK::svg << Value << ss.str();
            }
            out << EndMap;
        }
        out << EndSeq;
    }

    // End document
    out << EndMap << EndDoc;

    QFile outFile(file);
    outFile.open(QFile::WriteOnly);
    outFile.write(out.c_str());
    outFile.close();
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
