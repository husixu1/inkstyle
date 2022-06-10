#include "buttoninfo.hpp"

#include "constants.hpp"

#include <QRegularExpression>

void ButtonInfo::clear() {
    defIds.clear();
    customIconSvg.clear();
}

ButtonInfo::operator bool() const {
    return !isEmpty();
}

bool ButtonInfo::operator==(const ButtonInfo &other) const {
    return defIds == other.defIds && customIconSvg == other.customIconSvg;
}

ButtonInfo &ButtonInfo::operator+=(const ButtonInfo &other) {
    // Overwrite user-defined styles
    defIds.unite(other.defIds);
    customIconSvg = other.customIconSvg;
    return *this;
}

size_t ButtonInfo::hash(size_t seed) const {
    return qHash(defIds, seed) ^ qHash(customIconSvg, seed);
}

ButtonInfo::ButtonInfo(
    const QSet<QString> &defIds, const QByteArray &customIconSvg)
    : defIds(defIds), customIconSvg(customIconSvg) {}

QString ButtonInfo::genDefsSvg(const QHash<QString, QString> &svgDefs) const {
    // Recursively add svg definitions and avoid duplicates
    QString defs;
    QList<QString> idsToAdd(defIds.begin(), defIds.end());
    QSet<QString> idsAdded;
    for (auto itr = idsToAdd.begin(); itr != idsToAdd.end(); ++itr) {
        if (!svgDefs.contains(*itr))
            continue;

        defs.append(svgDefs.value(*itr));
        idsAdded.insert(*itr);

        static const QRegularExpression urlRegEx(
            R"-(\burl\((?<op>['"]?)#(?<id>.*?)\k<op>\))-");
        static const QRegularExpression hrefRegEx(
            R"-(\bxlink:href=(?<op>['"]?)(?<id>.*?)\k<op>)-");

        for (const QRegularExpression &re : {urlRegEx, hrefRegEx}) {
            auto matchIter = re.globalMatch(svgDefs.value(*itr));
            while (matchIter.hasNext()) {
                QRegularExpressionMatch match = matchIter.next();
                if (!idsAdded.contains(match.captured("id")))
                    idsToAdd.append(match.captured("id"));
            }
        }
    }
    return defs;
}

const QByteArray &ButtonInfo::getIconSvg() const {
    return customIconSvg;
}

void CustomButtonInfo::clear() {
    ButtonInfo::clear();
    customStyleSvg.clear();
}

bool CustomButtonInfo::isEmpty() const {
    return customStyleSvg.isEmpty();
}

bool CustomButtonInfo::operator==(const CustomButtonInfo &other) const {
    return this->ButtonInfo::operator==(other)
           && customStyleSvg == other.customStyleSvg;
}

CustomButtonInfo &CustomButtonInfo::operator+=(const CustomButtonInfo &other) {
    this->ButtonInfo::operator+=(other);
    customStyleSvg = other.customStyleSvg;
    return *this;
}

size_t CustomButtonInfo::hash(size_t seed) const {
    return this->ButtonInfo::hash(seed) ^ qHash(customStyleSvg, seed);
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
    : ButtonInfo(defIds, customIconSvg), customStyleSvg(customStyleSvg) {}

const QByteArray &CustomButtonInfo::getStyleSvg() const {
    return customStyleSvg;
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
    const QSet<QString> &defIds, const Config::StylesList &styles,
    const QByteArray &customIconSvg)
    : ButtonInfo(defIds, customIconSvg), styleList(styles) {}

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
