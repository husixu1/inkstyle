#include "configmanager.hpp"

#include "constants.hpp"

#include <QByteArray>
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
} // namespace YAML

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

            QMap<QString, QString> styles;
            QStringList styleList;
            QByteArray styleSvg;

            if (button[BK::customStyle].IsDefined()) {
                // Load non-standard styles
                styleSvg = button[BK::customStyle].as<QString>().toUtf8();
            } else {
                // Load standard styles
                static const char *standardStyles[] = {
                    BK::stroke,      BK::strokeWidth, BK::strokeDashArray,
                    BK::strokeStart, BK::strokeEnd,   BK::fill,
                    BK::fillOpacity, BK::fillGradient};
                for (const char *style : standardStyles)
                    if (button[style].IsDefined()) {
                        styles.insert(
                            QString(style), button[style].as<QString>());
                        styleList.append(
                            QString(style) + ":" + button[style].as<QString>());
                    }

                // Compose styles to svg
                styleSvg =
                    QString(C::ST::plainStyleTemplate)
                        .replace(C::ST::stylePlaceHolder, styleList.join(";"))
                        .toUtf8();
            }

            QByteArray icon;
            if (button[BK::customIcon].IsDefined())
                icon = button[icon].as<QString>().toUtf8();

            buttons.insert(
                slot, QSharedPointer<ButtonInfo>(
                          new ButtonInfo(styleSvg, icon, styles)));
            ++numButtons;
        }
    }
}

ConfigManager::ConfigManager(const QString &file, QObject *parent)
    : QObject(parent) {

    // Parse default config
    qDebug() << "Loading default config.";
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
        qWarning() << "No config file found. Using default config.";
    }
}

QByteArray ConfigManager::ButtonInfo::genIconSvg(
    QSizeF size, const PresetIconStyle &presetStyle) const {
    using namespace C::CK::BK;

    // Return custom icon if defined
    if (userIconSvg.size() > 0)
        return userIconSvg;

    QString iconTemplate;
    switch (presetStyle) {
    case PresetIconStyle::circle:
        iconTemplate = QString(
            R"-(<svg width="{W}" height="{H}" version="1.1")-"
            R"-( viewBox="0 0 {W} {H}" xmlns="http://www.w3.org/2000/svg">)-"
            R"-(  <circle cx="{CX}" cy="{CY}" r="{R}" style="{STYLE}"/>)-"
            R"-(</svg>)-");
        iconTemplate.replace("{W}", QString::number(size.width()));
        iconTemplate.replace("{H}", QString::number(size.height()));
        iconTemplate.replace("{CX}", QString::number(size.width() / 2.));
        iconTemplate.replace("{CY}", QString::number(size.height() / 2.));
        iconTemplate.replace(
            "{R}", QString::number(qMin(size.height(), size.width()) * 0.45));
        break;
    default:
        return QByteArray();
    }

    QStringList styleString;
    // set default fill
    if (!styles.contains(fill))
        styleString.append("fill:none");
    // set default stroke
    if (!styles.contains(stroke) && styles.contains(strokeWidth))
        styleString.append("stroke:#fff");

    for (auto iter = styles.constKeyValueBegin();
         iter != styles.constKeyValueEnd(); ++iter)
        styleString.append(iter->first + ":" + iter->second);
    iconTemplate.replace("{STYLE}", styleString.join(";"));

    return iconTemplate.toUtf8();
}

ConfigManager::ButtonInfo::ButtonInfo(
    const QByteArray &styleSvg, const QByteArray &userIconSvg,
    const QMap<QString, QString> &styles)
    : styleSvg(styleSvg), userIconSvg(userIconSvg), styles(styles) {}

ConfigManager::Slot ConfigManager::calcSlot(
    quint8 pSlot, quint8 tSlot, quint8 rSlot, quint8 subSlot) {
    return quint32(pSlot) << 24 | quint32(tSlot) << 16 | quint32(rSlot) << 8
           | quint32(subSlot);
}
