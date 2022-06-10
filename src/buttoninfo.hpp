#ifndef BUTTONINFO_HPP
#define BUTTONINFO_HPP

#include "config.hpp"
#include "visitorpattern.hpp"

#include <QHash>
#include <QSet>
#include <QString>

class StandardButtonInfo;
class CustomButtonInfo;
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

    const QByteArray &getStyleSvg() const;
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

#endif // BUTTONINFO_HPP
