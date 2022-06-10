#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include "visitorpattern.hpp"

#include <QColor>
#include <QHash>
#include <QIcon>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QVector>
#include <functional>
#include <yaml-cpp/yaml.h>

class CustomButtonInfo;
class StandardButtonInfo;

class Config : public QObject {
    Q_OBJECT

public:
    typedef quint32 Slot;
    typedef QMap<QString, QString> StylesList;

    static Slot
    calcSlot(quint8 pSlot, quint8 tSlot, quint8 rSlot, quint8 subSlot);

    bool hasButton(const Slot &slot) const;
    bool hasCustomButton(const Slot &slot) const;
    bool hasStandardButton(const Slot &slot) const;
    CustomButtonInfo getCustomButton(const Slot &slot) const;
    StandardButtonInfo getStandardButton(const Slot &slot) const;

    explicit Config(const QString &file, QObject *parent = nullptr);

    const QHash<QString, QString> &getSvgDefs() const;

    QColor panelBgColor;
    QColor buttonBgColorInactive;
    QColor buttonBgColorActive;
    QColor guideColor;
    quint8 panelMaxLevels;
    quint32 panelRadius;
    QString defaultIconStyle;
    QString defaultIconText;

private:
    /// @brief A list of buttons
    QHash<Slot, CustomButtonInfo> customButtons;
    QHash<Slot, StandardButtonInfo> standardButtons;

    /// @brief A list of svg defs (e.g. gradient, pattern, marker)
    /// @details stores: {defId, def-content}...
    QHash<QString, QString> svgDefs;

    void parseConfig(const YAML::Node &config);
    void parseGlobalConfig(const YAML::Node &config);
    void parseSvgDefsConfig(const YAML::Node &config);
    void parseButtonsConfig(const YAML::Node &config);
};

using ButtonInfoVisitor = Visitor<StandardButtonInfo, CustomButtonInfo>;

class ButtonInfo : public Visitable<ButtonInfoVisitor> {
public:
    virtual void clear();

    /// @brief Tell if there're styles stored in this button info
    virtual bool isEmpty() const = 0;

    /// @brief Alias to !isEmpty()
    operator bool() const;

    bool operator==(const ButtonInfo &other) const;

    /// @brief Merge two button info object
    ButtonInfo &operator+=(const ButtonInfo &other);

    /// @brief Generate hash to use as keys of QMap
    /// @return The hashed ButtonInfo
    virtual size_t hash(size_t seed = 0) const;

    ButtonInfo() = default;
    explicit ButtonInfo(const QSet<QString> &defIds);

    /// @brief Generate the style svg for copying to clipboard
    /// @param svgDefs A map that map the id of defs to their bodies
    virtual QByteArray
    genStyleSvg(const QHash<QString, QString> &svgDefs) const = 0;

    /// @brief Generate the defs svg to be used in `<defs></defs>` block
    /// @param svgDefs A map that map the id of defs to their bodies
    QString genDefsSvg(const QHash<QString, QString> &svgDefs) const;

private:
    /// @brief Ids of the svg definitions used by this button
    /// @see Config::svgDefs
    QSet<QString> defIds;
};

class CustomButtonInfo : public ButtonInfo {
public:
    virtual void clear() override;
    virtual bool isEmpty() const override;
    bool operator==(const CustomButtonInfo &other) const;
    CustomButtonInfo &operator+=(const CustomButtonInfo &other);
    virtual size_t hash(size_t seed = 0) const override;
    virtual QByteArray
    genStyleSvg(const QHash<QString, QString> &svgDefs) const override;
    virtual void accept(ButtonInfoVisitor &visitor) override;

    CustomButtonInfo() = default;
    CustomButtonInfo(
        const QSet<QString> &defIds, const QByteArray &customStyleSvg,
        const QByteArray &customIconSvg);

    const QByteArray &getIconSvg() const;

private:
    /// @brief This is a static svg for copy-pasting
    QByteArray customStyleSvg;

    /// @brief The icon svg provided by the user
    QByteArray customIconSvg;
};

class StandardButtonInfo : public ButtonInfo {
public:
    virtual void clear() override;
    virtual bool isEmpty() const override;
    bool operator==(const StandardButtonInfo &other) const;
    StandardButtonInfo &operator+=(const StandardButtonInfo &other);
    virtual size_t hash(size_t seed = 0) const override;
    virtual QByteArray
    genStyleSvg(const QHash<QString, QString> &svgDefs) const override;
    virtual void accept(ButtonInfoVisitor &visitor) override;

    StandardButtonInfo() = default;
    StandardButtonInfo(
        const QSet<QString> &defIds, const Config::StylesList &styles);

    const Config::StylesList &styles() const;

private:
    /// @brief A list of key-value pairs
    /// @details Key will be one of the constant in #C::CK::BK
    Config::StylesList styleList;
};

size_t qHash(const Config::StylesList &styles, size_t seed = 0);
size_t qHash(const ButtonInfo &info, size_t seed = 0);

#endif // CONFIGMANAGER_HPP
