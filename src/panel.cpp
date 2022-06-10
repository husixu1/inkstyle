#include "panel.hpp"

#include "constants.hpp"
#include "pugixml.hpp"

#include <QApplication>
#include <QCache>
#include <QClipboard>
#include <QCursor>
#include <QFile>
#include <QLayout>
#include <QMimeData>
#include <QMoveEvent>
#include <QPainter>
#include <QPalette>
#include <QPolygon>
#include <QPolygonF>
#include <QPushButton>
#include <QRegion>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <QVector>
#include <QtDebug>
#include <QtMath>
#include <algorithm>
#include <sstream>

uint qHash(const QPoint &point, uint seed = 0) {
    return qHash(QPair<int, int>(point.x(), point.y()), seed);
}

static QString _genQuestionMarkSvg(Button *button, qreal baselineHeight) {
    QSizeF size = button->inactiveGeometry.size() * button->hoverScale;
    return QString(R"(<text x="%1" y="%2" fill="#fff" style="%3">?</text>)")
        .arg(size.width() * 0.5)
        .arg(baselineHeight)
        .arg(QString("font-size:%1;text-anchor:middle")
                 .arg(size.height() * 0.5));
}

static QString _genSvgDefs(const Configs &conf, const ButtonInfo &info) {
    QString svgDefs = info.genDefsSvg(conf.getSvgDefs());
    // Replace non-displayable attributes so that markers can be displayed.
    static QRegularExpression _ctx_re(R"(\b(context-stroke|context-fill)\b)");
    svgDefs.replace(_ctx_re, "#fff");
    return svgDefs;
}

static QString _genColorSvg(
    const Configs &configs, const StandardButtonInfo &info, const QPointF &bl,
    const QPointF &tr, qreal radius) {
    namespace IC = C::IC;
    namespace CBK = C::C::B::K;
    namespace DIS = C::C::G::V::DIS;
    auto has = [&](const char *k) -> bool { return info.styles().contains(k); };

    // Set geometry for the svg element
    QString element;
    if (configs.defaultIconStyle == DIS::circle) {
        element =
            QString(R"(<path d="M %1 %2 A %3 %3 0 0 0 %4 %5" style="{S}"/>)")
                .arg(tr.x())
                .arg(tr.y())
                .arg(radius)
                .arg(bl.x())
                .arg(bl.y());
    } else if (configs.defaultIconStyle == DIS::square) {
        element = QString(R"(<path d="M %1 %2 H %3 V %4" style="{S}"/>)")
                      .arg(tr.x())
                      .arg(tr.y())
                      .arg(bl.x())
                      .arg(bl.y());
    }

    // Set style for color template
    QStringList styleString;
    // Set some defaults
    if (!has(CBK::fill))
        styleString.append("fill:none");
    if (has(CBK::stroke))
        styleString.append(QString("stroke-width:%1").arg(IC::strokeWidth));
    for (const char *key :
         {CBK::fill, CBK::stroke, CBK::strokeDashArray, CBK::strokeDashOffset})
        if (has(key))
            styleString.append(key + (":" + info.styles()[key]));
    return element.replace("{S}", styleString.join(";"));
}

static QPair<QString, QString> _genOpacitySvg(
    const Configs &configs, const StandardButtonInfo &info, const QPointF &bl,
    const QPointF &tr, qreal radius) {
    namespace IC = C::IC;
    namespace CBK = C::C::B::K;
    namespace DIS = C::C::G::V::DIS;
    auto has = [&](const char *k) -> bool { return info.styles().contains(k); };

    QString checkerboard =
        QString(R"(<pattern id="__checkerboard" patternUnits="userSpaceOnUse")"
                R"( width="%1" height="%1">)"
                R"(  <rect x="0" y="0" width="%2" height="%2" fill="#777"/>)"
                R"(  <rect x="0" y="%2" width="%2" height="%2" fill="#fff"/>)"
                R"(  <rect x="%2" y="%2" width="%2" height="%2" fill="#777"/>)"
                R"(  <rect x="%2" y="0" width="%2" height="%2" fill="#fff"/>)"
                R"(</pattern>)")
            .arg(2 * IC::checkerboardWidth)
            .arg(IC::checkerboardWidth);
    QString svgDefs = checkerboard;

    QString background, element, svgContent;
    if (configs.defaultIconStyle == DIS::circle) {
        background = element =
            QString(R"(<path d="M %1 %2 A %3 %3 0 0 0 %4 %5" style="{S}"/>)")
                .arg(bl.x())
                .arg(bl.y())
                .arg(radius)
                .arg(tr.x())
                .arg(tr.y());
    } else if (configs.defaultIconStyle == DIS::square) {
        background = element =
            QString(R"(<path d="M %1 %2 H %3 V %4" style="{S}"/>)")
                .arg(bl.x())
                .arg(bl.y())
                .arg(tr.x())
                .arg(tr.y());
    }

    // Set style for color template
    QStringList styleString;
    QStringList bgStyleString;
    // Set some defaults
    if (!has(CBK::fillOpacity)) {
        styleString.append("fill-opacity:0");
        bgStyleString.append("fill-opacity:0");
    } else {
        if (!has(CBK::fill))
            styleString.append("fill:#fff");
        bgStyleString.append("fill:url(#__checkerboard)");
    }

    if (has(CBK::strokeOpacity)) {
        if (!has(CBK::stroke))
            styleString.append("stroke:#fff");
        styleString.append(QString("stroke-width:%1").arg(IC::strokeWidth));
        bgStyleString.append(QString("stroke-width:%1").arg(IC::strokeWidth));
        bgStyleString.append("stroke:url(#__checkerboard)");
    }

    background.replace("{S}", bgStyleString.join(';'));
    svgContent += background;

    for (const char *key :
         {CBK::fill, CBK::stroke, CBK::stroke, CBK::fillOpacity,
          CBK::strokeOpacity})
        if (has(key))
            styleString.append(key + (":" + info.styles()[key]));
    svgContent += element.replace("{S}", styleString.join(';'));
    return {svgDefs, svgContent};
}

static QString _genStrokeWidthSvg(
    const Configs &configs, const StandardButtonInfo &info, const QPointF &stl,
    const QPointF &str, qreal sradius) {
    namespace IC = C::IC;
    namespace CBK = C::C::B::K;
    namespace DIS = C::C::G::V::DIS;
    auto has = [&](const char *k) -> bool { return info.styles().contains(k); };

    QString element;
    if (configs.defaultIconStyle == DIS::circle) {
        element =
            QString(R"(<path d="M %1 %2 A %3 %3 0 0 0 %4 %5" style="{S}"/>)")
                .arg(str.x())
                .arg(str.y())
                .arg(sradius)
                .arg(stl.x())
                .arg(stl.y());
    } else if (configs.defaultIconStyle == DIS::square) {
        element = QString(R"(<path d="M %1 %2 H %3" style="{S}"/>)")
                      .arg(str.x())
                      .arg(str.y())
                      .arg(stl.x());
    }

    QStringList styleString;
    styleString.append("fill-opacity:0");
    styleString.append("stroke:#fff");
    styleString.append(
        QString("stroke-width:%1").arg(info.styles()[CBK::strokeWidth]));
    return element.replace("{S}", styleString.join(';'));
}

static QString _genStrokeCapSvg(
    const StandardButtonInfo &info, const QPointF &cb, const QPointF &cm,
    const QPointF &ce) {
    namespace IC = C::IC;
    namespace CBK = C::C::B::K;
    auto has = [&](const char *k) -> bool { return info.styles().contains(k); };

    QString element =
        QString(R"(<path d="M %1 %2 L %3 %4 L %5 %6" style="{S}"/>)")
            .arg(cb.x())
            .arg(cb.y())
            .arg(cm.x())
            .arg(cm.y())
            .arg(ce.x())
            .arg(ce.y());
    QStringList styleString;
    styleString.append("fill-opacity:0");
    styleString.append("stroke:#fff");
    styleString.append(QString("stroke-width:%1").arg(IC::strokeWidth));
    styleString.append(
        CBK::strokeLineCap + (":" + info.styles()[CBK::strokeLineCap]));
    return element.replace("{S}", styleString.join(';'));
}

static QString _genStrokeJoinSvg(
    const StandardButtonInfo &info, const QPointF &jb, const QPointF &jm,
    const QPointF &je) {
    namespace IC = C::IC;
    namespace CBK = C::C::B::K;
    auto has = [&](const char *k) -> bool { return info.styles().contains(k); };

    QString element =
        QString(R"(<path d="M %1 %2 L %3 %4 L %5 %6" style="{S}"/>)")
            .arg(jb.x())
            .arg(jb.y())
            .arg(jm.x())
            .arg(jm.y())
            .arg(je.x())
            .arg(je.y());
    QStringList styleString;
    styleString.append("fill-opacity:0");
    styleString.append("stroke:#fff");
    styleString.append(QString("stroke-width:%1").arg(IC::strokeWidth));
    if (has(CBK::strokeLineJoin))
        styleString.append(
            CBK::strokeLineJoin + (":" + info.styles()[CBK::strokeLineJoin]));
    return element.replace("{S}", styleString.join(';'));
}

static QString _genMarkerSvg(
    const StandardButtonInfo &info, const QPointF &mbl, const QPointF &mbr) {
    namespace CBK = C::C::B::K;
    auto has = [&](const char *k) -> bool { return info.styles().contains(k); };

    qreal start = mbl.x(), end = mbr.x(), mid = (mbl.x() + mbr.x()) / 2;
    if (!has(CBK::markerStart))
        start = mid;
    if (!has(CBK::markerEnd))
        end = mid;

    QString element = QString(R"(<path d="M %1 %2 H %3 H %4" style="{S}"/>)")
                          .arg(start)
                          .arg(mbl.y())
                          .arg(mid)
                          .arg(end);
    QStringList styleString;
    styleString.append("stroke-width:2");
    styleString.append("stroke:#fff");
    for (const char *key : {CBK::markerStart, CBK::markerEnd, CBK::markerMid})
        if (has(key))
            styleString.append(key + (":" + info.styles()[key]));
    return element.replace("{S}", styleString.join(';'));
}

static QString _genFontSvg(
    const Configs &configs, const StandardButtonInfo &info, const QSizeF &size,
    qreal baselineHeight) {
    namespace CBK = C::C::B::K;
    auto has = [&](const char *k) -> bool { return info.styles().contains(k); };

    QString element =
        QString(R"(<text x="%1" y="%2" fill="#fff" style="{S}">%3</text>)")
            .arg(size.width() * 0.5)
            .arg(baselineHeight)
            .arg(configs.defaultIconText);

    QStringList styleString;

    // Set default fsize
    styleString.append(QString("font-size:%1").arg(size.height() * 0.5));
    // Correctly position the text
    styleString.append("text-anchor:middle");
    styleString.append("fill:#fff");

    for (const char *key : {CBK::fontFamily, CBK::fontStyle})
        if (has(key))
            styleString.append(key + (":" + info.styles()[key]));
    return element.replace("{S}", styleString.join(';'));
}

static QString _genFontSizeSvg(
    const StandardButtonInfo &info, const QSizeF &size, qreal baselineHeight) {
    namespace CBK = C::C::B::K;
    QString element =
        QString(R"(<text x="%1" y="%2" fill="#fff" style="{S}">%3</text>)")
            .arg(size.width() * 0.6)
            .arg(baselineHeight)
            .arg(info.styles()[CBK::fontSize]);

    QStringList styleString;
    styleString.append(QString("font-size:%1").arg(size.height() * 0.15));
    styleString.append("font-family:sans-serif");
    styleString.append("text-anchor:begin");
    styleString.append("fill:#fff");

    return element.replace("{S}", styleString.join(';'));
}

static QString _composeSvg(
    const QSizeF &size, const QString &svgDefs, const QString &svgContent) {
    return QString(R"(<svg width="%1" height="%2" version="1.1")"
                   R"( viewBox="0 0 %1 %2" xmlns="http://www.w3.org/2000/svg">)"
                   R"( <defs>%3</defs>%4</svg>)")
        .arg(size.width())
        .arg(size.height())
        .arg(svgDefs, svgContent);
}

static QByteArray _genUnknownStyleSvg(Button *button, bool orientation) {
    QSizeF size = button->inactiveGeometry.size() * button->hoverScale;
    qreal baselineHeight = size.height() * (orientation ? 0.5 : 0.85);
    return _composeSvg(size, {}, _genQuestionMarkSvg(button, baselineHeight))
        .toUtf8();
}

static QByteArray _genStyleButtonSvg(
    Button *button, const Configs &configs, const StandardButtonInfo &info,
    bool orientation) {
    using C::R30, C::R60, C::R45, C::RAD;
    namespace CGK = C::C::G::K;      // button configs Keys
    namespace CBK = C::C::B::K;      // button configs Keys
    namespace DIS = C::C::G::V::DIS; // default icon style

    QSizeF size = button->inactiveGeometry.size() * button->hoverScale;

    QString svgDefs;
    QString svgContent;

    // 0. Generate a indicator if this is a non-standard style
    qreal baselineHeight = size.height() * (orientation ? 0.5 : 0.85);
    if (info.isEmpty())
        svgContent += _genQuestionMarkSvg(button, baselineHeight);

    // 1.1 Calculate common anchor points for subsequent drawing
    auto has = [&](const char *k) -> bool { return info.styles().contains(k); };
    // button's centroid
    QPointF c = button->centroid * button->hoverScale;
    // color indicator's anchor points
    QPointF tr, bl;
    // color/gradient indicator's radius
    qreal R = size.height() / 3. - C::IC::strokeWidth / 2.;
    // stroke-width
    qreal sR = size.height() * 11. / 24.;
    // marker indicator's radius
    qreal mR = size.height() / 3.;

    qreal invert = orientation ? -1 : 1;
    // qreal mR = size.height() * 3. / 8.;
    if (configs.defaultIconStyle == DIS::circle) {
        tr = c + QPointF(R * qCos(R60), -R * qSin(R60));
        bl = c + QPointF(-R * qCos(R60), R * qSin(R60));
    } else if (configs.defaultIconStyle == DIS::square) {
        tr = c + QPointF(R * qCos(R45), -R * qSin(R45));
        bl = c + QPointF(-R * qCos(R45), R * qSin(R45));
        sR *= 2. / qSqrt(3.);
    }
    // stroke-width indicator's anchor points
    QPointF str = c + QPointF(sR * qCos(R60), -sR * qSin(R60)) * invert;
    QPointF stl = c + QPointF(-sR * qCos(R60), -sR * qSin(R60)) * invert;
    // marker indicator's anchor points
    QPointF mbl = c + QPointF(-mR * qCos(R45), mR * qSin(R45) * invert);
    QPointF mbr = c + QPointF(mR * qCos(R45), mR * qSin(R45) * invert);
    // line-cap indicator's anchor points
    QPointF cb =
        c + QPointF(-mR * qSin(R45) / qTan(RAD(40)), mR * qSin(R45) * invert);
    QPointF cm =
        c + QPointF(-mR * qSin(R45) / qTan(RAD(30)), mR * qSin(R45) * invert);
    QPointF ce = cm + (cb - cm).x() * QPointF(qCos(R60), -qSin(R60) * invert);
    // line-join indicator's anchor points
    QPointF jb =
        c + QPointF(mR * qSin(R45) / qTan(RAD(40)), mR * qSin(R45) * invert);
    QPointF jm =
        c + QPointF(mR * qSin(R45) / qTan(RAD(30)), mR * qSin(R45) * invert);
    QPointF je = jm + (jb - jm).x() * QPointF(qCos(R60), qSin(R60) * invert);

    // 1.2 Add necessary definitions
    svgDefs += _genSvgDefs(configs, info);

    // 2. Draw the fill/stroke color/style indicator
    if (has(CBK::fill) || has(CBK::stroke) || has(CBK::strokeDashArray))
        svgContent += _genColorSvg(configs, info, bl, tr, R);

    // 3. Draw the stroke/fill opacity indicator
    if (has(CBK::strokeOpacity) || has(CBK::fillOpacity)) {
        auto [defs, content] = _genOpacitySvg(configs, info, bl, tr, R);
        svgDefs += defs;
        svgContent += content;
    }

    // 4. Draw stroke-width indicator
    if (has(CBK::strokeWidth))
        svgContent += _genStrokeWidthSvg(configs, info, stl, str, sR);

    // 5.1: draw stroke-linecap indicator
    if (has(CBK::strokeLineCap))
        svgContent += _genStrokeCapSvg(info, cb, cm, ce);

    // 5.2: draw stroke-linejoin indicator
    if (has(CBK::strokeLineJoin))
        svgContent += _genStrokeJoinSvg(info, jb, jm, je);

    // 6. Draw marker start/end/mid indicator
    if (has(CBK::markerStart) || has(CBK::markerMid) || has(CBK::markerEnd))
        svgContent += _genMarkerSvg(info, mbl, mbr);

    // 7. Draw font-style indicator
    if (has(CBK::fontFamily) || has(CBK::fontStyle))
        svgContent += _genFontSvg(
            configs, info, size, size.height() * (orientation ? 0.5 : 0.85));

    // 8. Draw font-size indicator
    if (has(CBK::fontSize))
        svgContent += _genFontSizeSvg(
            info, size, size.height() * (orientation ? 0.4 : 0.75));

    // Compose final icon
    return _composeSvg(size, svgDefs, svgContent).toUtf8();
}

QPixmap
Panel::drawStyleButtonIcon(quint8 tSlot, quint8 rSlot, quint8 subSlot) const {
    using Slot = Configs::Slot;
    using SBInfo = StandardButtonInfo;
    using CBInfo = CustomButtonInfo;

    // Cache resvgOptions to reduce system font loading time
    static std::unique_ptr<ResvgOptions> resvgOptions;
    // Qt's svg is too weak. It cannot render clip/pattern. we use resvg here.
    if (!resvgOptions) {
        resvgOptions = std::make_unique<ResvgOptions>();
        resvgOptions->loadSystemFonts();
        resvgOptions->setImageRenderingMode(
            resvg_image_rendering::RESVG_IMAGE_RENDERING_OPTIMIZE_QUALITY);
    }

    Slot slot = calcSlot(pSlot, tSlot, rSlot, subSlot);

    // Get button to draw icon
    Button *button = styleButtons[slot].get();
    // Draw with scaled size, otherwise icon won't scale well
    QSizeF iconSize = button->inactiveGeometry.size() * button->hoverScale;

    auto render = [&](QByteArray iconSvg) -> QPixmap * {
        ResvgRenderer renderer(iconSvg, *resvgOptions);
        if (!renderer.isValid()) {
            qCritical(
                "Invalid SVG generated from slot %#x:\n%s", slot,
                iconSvg.toStdString().c_str());
            return nullptr;
        }

        QImage icon = renderer.renderToImage(iconSize.toSize());
        return new QPixmap(QPixmap::fromImage(icon));
    };

    QByteArray iconSvg;
    // true = pointing up, false = pointing down
    bool orientation = (tSlot + subSlot) % 2;
    if (configs->hasStandardButton(slot)) {
        // Cache rendered icons and reuse them if configs not changed.
        // Stores `{{slot, buttonInfo}, icon}`. The key is used
        // to test the validity of the buttonInfo associated with `slot`.
        static QCache<QPair<Slot, SBInfo>, QPixmap> cache(C::iconCacheSize);
        StandardButtonInfo info = configs->getStandardButton(slot);

        // Reuse cached icon for speedup
        if (cache.contains({slot, info}))
            return *cache[{slot, info}];

        QPixmap *pixmap = render(
            info.getIconSvg().isEmpty()
                ? _genStyleButtonSvg(button, *configs, info, orientation)
                : info.getIconSvg());

        // Cache takes the ownership of pixmap. See QCache document.
        if (pixmap) {
            cache.insert({slot, info}, pixmap);
            return *pixmap;
        }
    } else if (configs->hasCustomButton(slot)) {
        static QCache<QPair<Slot, CBInfo>, QPixmap> cache(C::iconCacheSize);
        CustomButtonInfo info = configs->getCustomButton(slot);
        if (cache.contains({slot, info}))
            return *cache[{slot, info}];

        QPixmap *pixmap = render(
            info.getIconSvg().isEmpty()
                ? _genUnknownStyleSvg(button, orientation)
                : info.getIconSvg());
        if (pixmap) {
            cache.insert({slot, info}, pixmap);
            return *pixmap;
        }
    }
    return QPixmap();
}

static QByteArray _genCentralButtonSvg(
    Button *button, const Configs &configs, const StandardButtonInfo &info) {
    using C::R30, C::R60, C::R45, C::RAD;
    constexpr qreal R15 = RAD(15);
    namespace CGK = C::C::G::K;      // button configs Keys
    namespace CBK = C::C::B::K;      // button configs Keys
    namespace DIS = C::C::G::V::DIS; // default icon style

    // Button *button = styleButtons[slot].get();
    QSizeF size = button->inactiveGeometry.size() * button->hoverScale;
    QPointF c = button->centroid * button->hoverScale;

    QString svgDefs;
    QString svgContent;

    // 0. Generate a indicator if this is a non-standard style
    if (info.isEmpty())
        svgContent += _genQuestionMarkSvg(button, size.height() * 0.675);

    // 1.1 Calculate common anchor points for subsequent drawing
    auto has = [&](const char *k) -> bool { return info.styles().contains(k); };
    // color/stroke-width/marker indicator's top/bottom-left/right point
    QPointF tr, bl, str, stl, mbl, mbr;
    // color/gradient indicator's radius
    qreal R = size.height() / 3. - C::IC::strokeWidth / 2.;
    // stroke-width/marker indicator's radius
    qreal sR = size.height() * 11. / 24., mR = sR;
    // qreal mradius = size.height() * 3. / 8.;
    if (configs.defaultIconStyle == DIS::circle) {
        tr = c + QPointF(R * qCos(R60), -R * qSin(R60));
        bl = c + QPointF(-R * qCos(R60), R * qSin(R60));
        str = c + QPointF(sR * qCos(R30), -sR * qSin(R30));
        stl = c + QPointF(-sR * qCos(R30), -sR * qSin(R30));
        mbl = c + QPointF(-mR * qCos(R30), mR * qSin(R30));
        mbr = c + QPointF(mR * qCos(R30), mR * qSin(R30));
    } else if (configs.defaultIconStyle == DIS::square) {
        tr = c + QPointF(R * qCos(R45), -R * qSin(R45));
        bl = c + QPointF(-R * qCos(R45), R * qSin(R45));
        str = c + QPointF(sR * qCos(R45), -sR * qSin(R45));
        stl = c + QPointF(-sR * qCos(R45), -sR * qSin(R45));
        mbl = c + QPointF(-mR * qCos(R30), mR * qSin(R30));
        mbr = c + QPointF(mR * qCos(R30), mR * qSin(R30));
    };
    // line-cap indicator's anchor points
    QPointF cb =
        c + QPointF(-mR * qCos(R30), mR * qCos(R30) / qCos(R15) * qSin(R15));
    QPointF cm = c + QPointF(-mR, 0);
    QPointF ce =
        c + QPointF(-mR * qCos(R30), -mR * qCos(R30) / qCos(R15) * qSin(R15));
    // line-join indicator's anchor points
    QPointF jb =
        c + QPointF(mR * qCos(R30), mR * qCos(R30) / qCos(R15) * qSin(R15));
    QPointF jm = c + QPointF(mR, 0);
    QPointF je =
        c + QPointF(mR * qCos(R30), -mR * qCos(R30) / qCos(R15) * qSin(R15));

    // 1.2 Add necessary definitions
    svgDefs += _genSvgDefs(configs, info);

    // 2. Draw the fill/stroke color/style indicator
    if (has(CBK::fill) || has(CBK::stroke) || has(CBK::strokeDashArray))
        svgContent += _genColorSvg(configs, info, bl, tr, R);

    // 3. Draw the stroke/fill opacity indicator
    if (has(CBK::strokeOpacity) || has(CBK::fillOpacity)) {
        auto [defs, content] = _genOpacitySvg(configs, info, bl, tr, R);
        svgDefs += defs;
        svgContent += content;
    }

    // 4. Draw stroke-width indicator
    if (has(CBK::strokeWidth))
        svgContent += _genStrokeWidthSvg(configs, info, stl, str, sR);

    // 5.1: draw stroke-linecap indicator
    if (has(CBK::strokeLineCap))
        svgContent += _genStrokeCapSvg(info, cb, cm, ce);

    // 5.2: draw stroke-linejoin indicator
    if (has(CBK::strokeLineJoin))
        svgContent += _genStrokeJoinSvg(info, jb, jm, je);

    // 6. Draw marker start/end/mid indicator
    if (has(CBK::markerStart) || has(CBK::markerMid) || has(CBK::markerEnd))
        svgContent += _genMarkerSvg(info, mbl, mbr);

    // 7. Draw font-style indicator
    if (has(CBK::fontFamily) || has(CBK::fontStyle))
        svgContent += _genFontSvg(configs, info, size, size.height() * 0.675);

    // 8. Draw font-size indicator
    if (has(CBK::fontSize))
        svgContent += _genFontSizeSvg(info, size, size.height() * 0.575);

    // Compose final icon
    return _composeSvg(size, svgDefs, svgContent).toUtf8();
}

QPixmap Panel::drawCentralButtonIcon() const {
    // Cache rendered icons and reuse them if configs not changed.
    // Stores `{composed-style-list, icon}`.
    static QCache<StandardButtonInfo, QPixmap> cachedIcons(C::iconCacheSize);

    // Reuse cached icon for speedup
    QPixmap cachedIcon;
    bool hasCachedIcon = false;

    Button *button = centralButton.get();
    QByteArray iconSvg;
    centralButtonInfo->accept(ButtonInfoVisitor{
        [&](StandardButtonInfo &info) {
            if (cachedIcons.contains(info)) {
                cachedIcon = *cachedIcons[info];
                hasCachedIcon = true;
            } else {
                // Redraw icon
                iconSvg = _genCentralButtonSvg(button, *configs, info);
            }
        },
        [&](CustomButtonInfo &info) { iconSvg = info.getIconSvg(); }});

    if (hasCachedIcon)
        return cachedIcon;

    // Draw with scaled size, otherwise icon won't scale well
    QSizeF iconSize = button->inactiveGeometry.size() * button->hoverScale;

    // Cache resvgOptions to reduce system font loading time
    static std::unique_ptr<ResvgOptions> resvgOptions;
    if (!resvgOptions) {
        resvgOptions = std::make_unique<ResvgOptions>();
        resvgOptions->loadSystemFonts();
        resvgOptions->setImageRenderingMode(
            resvg_image_rendering::RESVG_IMAGE_RENDERING_OPTIMIZE_QUALITY);
    }

    ResvgRenderer renderer(iconSvg, *resvgOptions);
    if (!renderer.isValid()) {
        qCritical(
            "Invalid SVG generated from central button:\n%s",
            iconSvg.toStdString().c_str());
        return QPixmap();
    }

    QImage icon = renderer.renderToImage(iconSize.toSize());
    QPixmap pixmap = QPixmap::fromImage(icon);
    // QCache takes the ownership of pixmap and might free memory immediately.

    centralButtonInfo->accept(ButtonInfoVisitor{
        [&](StandardButtonInfo &info) {
            cachedIcons.insert(info, new QPixmap(pixmap));
        },
        [&](CustomButtonInfo &) {}});
    return pixmap;
}

bool Panel::isActive() const {
    return bool(activeButtons.size())
           || std::any_of(
               childPanels.cbegin(), childPanels.cend(),
               [](const auto &panel) { return panel && panel->isActive(); });
}

void Panel::updateStyleFromClipboard(
    quint8 tSlot, quint8 rSlot, quint8 subSlot) {

    // Copy style associated with slot to clipboard
    const QMimeData *styleSvg = QApplication::clipboard()->mimeData();
    QString svg = styleSvg->data(C::styleMimeType);

    // Parse clipboard with pugixml
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(svg.toStdString().c_str());
    if (result) {
        // parse clipboard
        qDebug("clipboard: %s", svg.toStdString().c_str());
        const pugi::xml_node &node =
            doc.find_node([](const pugi::xml_node &node) -> bool {
                return std::string("inkscape:clipboard") == node.name();
            });

        // Parse style as a map of {key, value} pairs
        QHash<QString, QString> styles;
        for (QString &s : QString(node.attribute("style").value()).split(';'))
            styles.insert(s.split(':')[0].trimmed(), s.split(':')[1].trimmed());

        // Parse svg defs
        QHash<QString, QString> svgDefs;
        const pugi::xml_node &defs =
            doc.find_node([](const pugi::xml_node &node) -> bool {
                return std::string("defs") == node.name();
            });
        for (pugi::xml_node &def : defs.children()) {
            std::stringstream ss;
            def.print(ss);
            svgDefs.insert(
                def.attribute("id").value(), QString::fromStdString(ss.str()));
        }

        for (const QString &def : svgDefs)
            qDebug("%s", def.toStdString().c_str());

        // Store and save parsed styles into configs
        configs->updateGeneratedConfig(
            calcSlot(pSlot, tSlot, rSlot, subSlot), styles, svgDefs);

    } else {
        // Parse error
        qWarning(
            "xml parsed with errors: %s (at offset %ld).", result.description(),
            result.offset);
    }
}

Config::Slot
Panel::calcSlot(quint8 pSlot, quint8 tSlot, quint8 rSlot, quint8 subSlot) {
    return quint32(pSlot) << 24 | quint32(tSlot) << 16 | quint32(rSlot) << 8
           | quint32(subSlot);
}

QPoint Panel::calcRelativeCoordinate(quint8 tSlot) {
    Q_ASSERT(tSlot <= 5);
    switch (tSlot) {
    case 0:
        return coordinate + QPoint{1, 0};
    case 1:
        return coordinate + QPoint{0, 1};
    case 2:
        return coordinate + QPoint{-1, 1};
    case 3:
        return coordinate + QPoint{-1, 0};
    case 4:
        return coordinate + QPoint{0, -1};
    case 5:
        return coordinate + QPoint{1, -1};
    default:
        return QPoint{0, 0};
    }
}

QPoint Panel::calcRelativePanelPos(quint8 tSlot) {
    using C::R30, C::R60;
    return pos()
           + QPointF(
                 unitLen * qSqrt(3) * qCos(R30 + R60 * tSlot),
                 -unitLen * qSqrt(3) * qSin(R30 + R60 * tSlot))
                 .toPoint();
}

QVector<QPointF> Panel::genBorderButtonMask(quint8 tSlot) {
    using C::R30, C::R60;
    return {
        {
            size().width() / 2. + qCos(tSlot * R60) * unitLen,
            size().height() / 2. - qSin(tSlot * R60) * unitLen,
        },
        {
            size().width() / 2. + qCos((tSlot + 1) * R60) * unitLen,
            size().height() / 2. - qSin((tSlot + 1) * R60) * unitLen,
        },
        {
            size().width() / 2. + qCos((tSlot + 1) * R60) * unitLen * 4. / 3.,
            size().height() / 2. - qSin((tSlot + 1) * R60) * unitLen * 4. / 3.,
        },
        {
            size().width() / 2. + qCos(tSlot * R60) * unitLen * 4. / 3.,
            size().height() / 2. - (qSin(tSlot * R60) * unitLen * 4. / 3.),
        }};
}

QVector<QPointF>
Panel::genStyleButtonMask(quint8 tSlot, quint8 rSlot, quint8 subSlot) {
    using C::R30, C::R60;

    // Upper half of the hexagon:
    /*
    **       •---•---•---•
    **      / \ / \ / \ / \
    **     •---•---•---•---•
    **    / \ / \ / \ / \ / \
    **   •---•---•---•---2---3
    **  / \ / \ / \ / \ / \ / \
    ** •---•---•---C-->•-->1-->2
    */
    // -->: the tSlot direction
    // C: center of the hexagon
    // •: triangle vertices
    // 1: the 1st point
    // 2: the 2nd point
    // 3: the 3rd point

    // Vertex of the triangular button (coordinates relative to panel)
    return {
        {
            // The 1st point
            size().width() / 2.
                // Base x
                + ((rSlot * qCos(tSlot * R60)
                    + (subSlot / 2) * qCos((tSlot + 2) * R60))
                       * unitLen / 3.
                   // Offset for border
                   + gapLen * qCos((tSlot + 0.5 + (subSlot % 2)) * R60)),
            size().height() / 2.
                // Base y
                - ((rSlot * qSin(tSlot * R60)
                    + (subSlot / 2) * qSin((tSlot + 2) * R60))
                       * unitLen / 3.
                   // Offset for border
                   + gapLen * qSin((tSlot + 0.5 + (subSlot % 2)) * R60)),
        },
        {
            // The 2nd point
            size().width() / 2.
                + (((rSlot + 1 - (subSlot % 2)) * qCos(tSlot * R60)
                    + ((subSlot + 1) / 2) * qCos((tSlot + 2) * R60))
                       * unitLen / 3.
                   + gapLen * qCos((tSlot + 2.5 - (subSlot % 2) * 3) * R60)),
            size().height() / 2.
                - (((rSlot + 1 - (subSlot % 2)) * qSin(tSlot * R60)
                    + ((subSlot + 1) / 2) * qSin((tSlot + 2) * R60))
                       * unitLen / 3.
                   + gapLen * qSin((tSlot + 2.5 - (subSlot % 2) * 3) * R60)),
        },
        {
            // The 3rd point
            size().width() / 2.
                + (((rSlot + 1) * qCos(tSlot * R60)
                    + (subSlot / 2 + 1) * qCos((tSlot + 2) * R60))
                       * unitLen / 3.
                   + gapLen * qCos((tSlot - 1.5 - (subSlot % 2)) * R60)),
            size().height() / 2.
                - (((rSlot + 1) * qSin(tSlot * R60)
                    + (subSlot / 2 + 1) * qSin((tSlot + 2) * R60))
                       * unitLen / 3.
                   + gapLen * qSin((tSlot - 1.5 - (subSlot % 2)) * R60)),
        }};
}

QVector<QPointF> Panel::genCentralButtonMask() {
    using C::R60;

    QVector<QPointF> points;
    for (int i = 0; i < 6; ++i)
        points.append(
            {size().width() / 2.
                 + (unitLen / 3. - gapLen * qSin(R60)) * qCos(R60 * i),
             size().height() / 2.
                 - (unitLen / 3. - gapLen * qSin(R60)) * qSin(R60 * i)});
    return points;
}

Panel::Panel(
    Panel *parent, quint8 tSlot, const QSharedPointer<Configs> &configs)
    : QWidget(nullptr), configs(parent ? parent->configs : configs),
      _pGrid(parent ? parent->_pGrid : QSharedPointer<PGrid>(new PGrid)),
      panelGrid(*_pGrid),
      coordinate(parent ? parent->calcRelativeCoordinate(tSlot) : QPoint{0, 0}),
      pSlot(parent ? parent->parentPanel ? parent->pSlot + 6 : tSlot + 1 : 0),
      parentPanel(parent), tSlot(tSlot), childPanels(6, nullptr),
      borderButtons(6, nullptr), hoverScale(1.5), unitLen(200), gapLen(3) {
    using C::R60;

    // Preconditions
    Q_ASSERT_X(this->configs, __func__, "Configs not initialized");
    Q_ASSERT_X(this->_pGrid, __func__, "Panel grid not initialized");

    // Set common window attributes
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlag(Qt::FramelessWindowHint);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowFlag(Qt::NoDropShadowWindowHint);

    // Add panel to grid
    panelGrid[coordinate] = this;

    // Set window location and size to be the bounding box of the hexagon
    setFixedSize(
        int(unitLen * (2 + 2 / 3.)), int(unitLen * qSin(R60) * (2 + 2 / 3.)));

    // Show before move to allow creating a window outside the screen
    show();

    if (parentPanel)
        move(parentPanel->calcRelativePanelPos(tSlot));
    else {
        QPoint center(geometry().width() / 2, geometry().height() / 2);
        move(QCursor::pos() - center);
    }

    // Add style buttons
    for (quint8 i = 0; i < 6; ++i)
        for (quint8 j = (parentPanel ? 0 : 1); j <= 2; ++j)
            for (quint8 k = 0; k < j * 2 + 1; ++k)
                addStyleButton(i, j, k);

    // Add border buttons if not exceeding max levels, skip the occupied edge
    for (quint8 i = 0; i < 6; ++i)
        if (!parentPanel || !panelGrid.contains(calcRelativeCoordinate(i)))
            addBorderButton(i);

    updateMask();
}

void Panel::updateMask() {
    using C::R60;
    // Generate the center hexagon
    QPolygon polygon;
    QVector<int> points;
    for (size_t i = 0; i < 6; ++i) {
        points.append(
            {int(size().width() / 2.
                 + qCos(R60 * i)
                       * (unitLen * (1. + (hoverScale - 1.) / 6.)
                          /*- gapLen * (hoverScale - 1.) * qCos(R60)*/)),
             int(size().height() / 2
                 + qSin(R60 * i)
                       * (unitLen * (1. + (hoverScale - 1.) / 6.)
                          /* gapLen * (hoverScale - 1.) * qCos(R60)*/))});
    }
    polygon.setPoints(6, points.data());
    QRegion mask(polygon);

    // add border-button masks
    for (int tSlot = 0; tSlot < borderButtons.size(); ++tSlot)
        if (borderButtons[tSlot])
            mask += QRegion(QPolygonF(genBorderButtonMask(tSlot)).toPolygon());

    // Set new mask
    clearMask();
    setMask(mask);
}

Button *Panel::addStyleButton(quint8 tSlot, quint8 rSlot, quint8 subSlot) {
    Q_ASSERT(tSlot <= 5);
    Q_ASSERT(rSlot <= 2);
    Q_ASSERT(subSlot <= rSlot * 2);

    QVector<QPointF> points = genStyleButtonMask(tSlot, rSlot, subSlot);
    QPointF centroid = std::reduce(points.begin(), points.end()) / 3.;
    QPolygonF mask(points);
    QRectF geometry(mask.boundingRect());
    mask.translate(-geometry.topLeft());

    // Add the button to styleButtons
    Configs::Slot slot = calcSlot(pSlot, tSlot, rSlot, subSlot);
    QSharedPointer<Button> button(
        new Button(
            geometry, mask, hoverScale, centroid - geometry.topLeft(), this,
            configs->buttonBgColorInactive, configs->buttonBgColorActive),
        [](Button *b) { b->deleteLater(); });
    styleButtons.insert(slot, button);

    // Draw icon on the button
    if (configs->hasButton(slot)) {
        // Draw and set icon for this button
        button->setIcon(drawStyleButtonIcon(tSlot, rSlot, subSlot));
        button->setIconSize(button->geometry().size());
    }
    button->show();

    Button *rawButton = button.get();

    // Raise panel on mouseEnter for better looks
    connect(rawButton, &Button::mouseEnter, this, &QWidget::raise);
    // Make the button toggle-able
    connect(rawButton, &QPushButton::clicked, rawButton, &Button::toggle);
    // Enable button replacement (update style from clipboard)
    connect(
        rawButton, &Button::stateUpdated, this,
        [this, rawButton, tSlot, rSlot, subSlot] {
            // update config and store config to file
            updateStyleFromClipboard(tSlot, rSlot, subSlot);
            configs->saveGeneratedConfig();

            // update displayed button
            delStyleButton(tSlot, rSlot, subSlot);
            addStyleButton(tSlot, rSlot, subSlot);

            qDebug() << rawButton << " style updated";
        });

    for (Panel *panel = this; panel; panel = panel->parentPanel) {
        // Set composed styles and central icons
        auto updateStyles = [this, slot, panel] {
            // Don't smart pointer here, otherwise will cause smart pointer
            // loop
            auto action = (this->styleButtons[slot]->isActive()
                           || this->styleButtons[slot]->isHovering())
                              ? &ActiveButtons::insert
                              : &ActiveButtons::remove;
            // Update styles and central icon of all parent panels
            (panel->activeButtons.*action)(slot);
            panel->composeCentralButtonInfo();
            if (!panel->parentPanel)
                panel->updateCentralButton();
        };
        connect(rawButton, &Button::mouseEnter, this, updateStyles);
        connect(rawButton, &Button::mouseLeave, this, updateStyles);
        connect(rawButton, &QPushButton::clicked, this, updateStyles);
    }

    return button.get();
}

void Panel::delStyleButton(quint8 tSlot, quint8 rSlot, quint8 subSlot) {
    Configs::Slot slot = calcSlot(pSlot, tSlot, rSlot, subSlot);
    if (styleButtons.contains(slot)) {
        if (styleButtons[slot]) {
            styleButtons[slot]->disconnect();
            styleButtons[slot] = nullptr;
        }
        styleButtons.remove(slot);
    }
}

HiddenButton *Panel::addBorderButton(quint8 tSlot) {
    Q_ASSERT(tSlot <= 5);

    if (borderButtons[tSlot])
        return nullptr;

    // Vertices of the trapezoidal button
    QVector<QPointF> points = genBorderButtonMask(tSlot);
    QPolygonF mask(points);
    QRectF geometry = mask.boundingRect();
    mask.translate(-geometry.topLeft());

    // Using delayed delete is necessary since it is possible that the
    // deletion is triggered during the button's own event
    QSharedPointer<HiddenButton> newButton(
        new HiddenButton(this), [](HiddenButton *b) { b->deleteLater(); });

    // Add button to UI
    borderButtons[tSlot] = newButton;
    newButton->setGeometry(geometry.toRect());
    newButton->setMask(mask.toPolygon());
    newButton->show();

    // Connect button functions
    connect(newButton.get(), &HiddenButton::mouseEnter, this, [this, tSlot] {
        if ((pSlot - 1) / 6 < configs->panelMaxLevels - 1)
            Panel::addPanel(tSlot);
    });
    return borderButtons[tSlot].data();
}

void Panel::delBorderButton(quint8 tSlot) {
    borderButtons[tSlot] = nullptr;
}

void Panel::updateCentralButton() {
    // Remove central button if no icon available / is not root panel
    if (!centralButtonInfo || centralButtonInfo->isEmpty()) {
        centralButton = nullptr;
        return;
    }

    QVector<QPointF> points = genCentralButtonMask();
    QPointF centroid = std::reduce(points.begin(), points.end()) / 6.;
    QPolygonF mask(points);
    QRectF geometry = mask.boundingRect();
    mask.translate(-geometry.topLeft());

    // Add a central button if not exist
    if (!centralButton) {
        centralButton = QSharedPointer<Button>(
            new Button(
                geometry, mask, hoverScale, centroid - geometry.topLeft(), this,
                configs->buttonBgColorInactive, configs->buttonBgColorActive),
            [](Button *button) { button->deleteLater(); });
        centralButton->show();
    }

    // Draw and set icon for this button
    centralButton->setIcon(drawCentralButtonIcon());
    centralButton->setIconSize(centralButton->geometry().size());

    // Raise on mouseEnter for better looking
    connect(centralButton.get(), &Button::mouseEnter, this, &QWidget::raise);
}

void Panel::composeCentralButtonInfo() {
    // Compose styles from all active buttons
    bool isStandardStyle = true;
    QSharedPointer<StandardButtonInfo> standardButton(new StandardButtonInfo);
    QSharedPointer<CustomButtonInfo> customButton(new CustomButtonInfo);

    for (const Configs::Slot &slot : activeButtons.orderedList())
        if (isStandardStyle && configs->hasStandardButton(slot)) {
            *standardButton += configs->getStandardButton(slot);
        } else if (configs->hasCustomButton(slot)) {
            *customButton += configs->getCustomButton(slot);
            isStandardStyle = false;
        }

    isStandardStyle ? (centralButtonInfo = standardButton)
                    : (centralButtonInfo = customButton);
}

void Panel::moveEvent(QMoveEvent *event) {
    // Prevent infinite recursion.
    if (pos() == event->oldPos())
        return;

    // Find top-level panel
    for (int i = 0; i < childPanels.size(); ++i)
        if (childPanels[i])
            childPanels[i]->move(calcRelativePanelPos(i));

    if (parentPanel)
        parentPanel->move(calcRelativePanelPos((tSlot + 3) % 6));
}

void Panel::closeEvent(QCloseEvent *e) {
    // Unregister from panelGrid first to avoid subsequent neigbor processing
    panelGrid.remove(coordinate);

    // Disconnect all buttons to prevent subsequent unwanted events
    for (const auto &button : qAsConst(styleButtons))
        if (button)
            button->disconnect();
    for (const auto &button : qAsConst(borderButtons))
        if (button)
            button->disconnect();
    if (centralButton)
        centralButton->disconnect();

    // Close all child panels
    childPanels.fill(nullptr);

    // Restore border buttons of neighboring panels
    for (quint8 tSlot = 0; tSlot < 6; ++tSlot)
        if (QPoint coordinate = calcRelativeCoordinate(tSlot);
            panelGrid.contains(coordinate)) {
            Panel *panel = panelGrid[coordinate];
            panel->addBorderButton((tSlot + 3) % 6);
            panel->updateMask();
        }

    QWidget::closeEvent(e);
}

void Panel::paintEvent(QPaintEvent *) {
    using C::R60;

    QPainter painter(this);
    painter.setRenderHints(
        QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
    painter.setPen(QPen(Qt::white, gapLen - 1, Qt::SolidLine, Qt::RoundCap));
    QPointF center(geometry().width() / 2, geometry().height() / 2);

    if (!parentPanel) {
        // For root panel, divide to 6 fans
        QVector<QLineF> lines;
        for (quint8 i = 0; i < 6; ++i)
            lines.append({
                center
                    + QPointF(
                        qCos(i * R60) * unitLen / 3.,
                        -qSin(i * R60) * unitLen / 3.),
                center
                    + QPointF(
                        qCos(i * R60) * unitLen, -qSin(i * R60) * unitLen),
            });
        painter.drawLines(lines);
    } else {
        // For child panels, draw on borders
        QVector<QLineF> lines;
        for (quint8 i = 0; i < 6; ++i)
            if (!((i + 3) % 6 == tSlot || childPanels[i]))
                lines.append({
                    center
                        + QPointF(
                            qCos(i * R60) * unitLen, -qSin(i * R60) * unitLen),
                    center
                        + QPointF(
                            qCos((i + 1) * R60) * unitLen,
                            -qSin((i + 1) * R60) * unitLen),
                });
        painter.drawLines(lines);
    }
}

void Panel::enterEvent(QEvent *e) {
    // Close all inactive child panels
    for (int tSlot = 0; tSlot < childPanels.size(); ++tSlot)
        if (childPanels[tSlot]) {
            if (!childPanels[tSlot]->isActive())
                childPanels[tSlot] = nullptr;
            else // Recursively call enterEvent to close all inactive
                 // children
                childPanels[tSlot]->enterEvent(e);
        }
}

void Panel::addPanel(quint8 tSlot) {
    Q_ASSERT(tSlot <= 5);

    // Make sure the panel is only added once
    if (childPanels[tSlot])
        return;

    QSharedPointer<Panel> panel(new Panel(this, tSlot), [this](Panel *panel) {
        panel->close();
        panel->deleteLater();
    });
    childPanels[tSlot] = panel;
    panel->show();
    panel->move(childPanels[tSlot]->pos());
    qDebug() << "Added panel " << panel->pSlot;

    // Update neighboring panels' border buttons and masks
    for (quint8 slot = 0; slot < 6; ++slot) {
        // Delete neighboring border buttons
        QPoint neighborCoordinate = panel->calcRelativeCoordinate(slot);
        if (panelGrid.contains(neighborCoordinate)) {
            panelGrid[neighborCoordinate]->delBorderButton((slot + 3) % 6);
            panelGrid[neighborCoordinate]->updateMask();
        }
    }

    // Update guides
    update();
}

void Panel::delPanel(quint8 tSlot) {
    Q_ASSERT(tSlot <= 5);
    if (!childPanels[tSlot])
        return;
    childPanels[tSlot] = nullptr;
}

void Panel::copyStyle() {
    if (centralButtonInfo && !centralButtonInfo->isEmpty()) {
        // Copy style associated with slot to clipboard
        QMimeData *styleSvg = new QMimeData;
        styleSvg->setData(
            C::styleMimeType,
            centralButtonInfo->genStyleSvg(configs->getSvgDefs()));
        QApplication::clipboard()->setMimeData(styleSvg);
        qDebug() << "Style copied " << styleSvg->data(C::styleMimeType);
    } else {
        qDebug() << "No style copied";
    }
}

void Panel::ActiveButtons::insert(const Configs::Slot &slot) {
    if (list.contains(slot))
        return;

    if (list.size()) {
        list[tail].second = slot;
        list.insert(slot, {tail, slot});
    } else {
        head = slot;
        list.insert(slot, {slot, slot});
    }
    tail = slot;
}

void Panel::ActiveButtons::remove(const Configs::Slot &slot) {
    if (!list.contains(slot))
        return;

    Configs::Slot prev = list[slot].first;
    Configs::Slot next = list[slot].second;

    if (prev != slot)
        list[prev].second = (next == slot ? prev : next);
    else
        head = next;

    if (next != slot)
        list[next].first = (prev == slot ? next : prev);
    else
        tail = prev;

    list.remove(slot);
}

qsizetype Panel::ActiveButtons::size() const {
    return list.size();
}

QList<Configs::Slot> Panel::ActiveButtons::orderedList() const {
    if (!list.size())
        return {};

    QList<Configs::Slot> result;
    for (Configs::Slot cur = head; cur != tail; cur = list[cur].second)
        result.append(cur);
    result.append(tail);
    return result;
}
