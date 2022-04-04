#include "panel.hpp"

#include "constants.hpp"

#include <QApplication>
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

decltype(Panel::cache) Panel::cache;

uint qHash(const QPoint &point, uint seed = 0) {
    return qHash(QPair<int, int>(point.x(), point.y()), seed);
}

QPixmap Panel::drawIcon(
    quint8 tSlot, quint8 rSlot, quint8 subSlot,
    const Config::ButtonInfo &info) const {

    Config::Slot slot = Config::calcSlot(pSlot, tSlot, rSlot, subSlot);

    // Reuse cached icon for speedup
    if (cache.styleIcons.contains(slot)
        && info.validateHash(cache.styleIcons[slot].first)) {
        return cache.styleIcons[slot].second;
    }

    // Redraw icon
    Button *button = styleButtons[slot].get();
    // Draw with scaled size, otherwise icon won't scale well
    QSizeF iconSize = button->inactiveGeometry.size() * button->hoverScale;

    // Qt's svg is too weak. It cannot render clip/pattern. we use resvg here.
    // Use cached resvg options
    if (!cache.resvgOptions) {
        cache.resvgOptions = std::make_unique<ResvgOptions>();
        cache.resvgOptions->loadSystemFonts();
        // cache.resvgOptions->setDpi(96);
        cache.resvgOptions->setImageRenderingMode(
            resvg_image_rendering::RESVG_IMAGE_RENDERING_OPTIMIZE_QUALITY);
    }

    QByteArray iconSvg = genIconSvg(tSlot, rSlot, subSlot, info);
    qDebug("%s\n", iconSvg.toStdString().c_str());
    ResvgRenderer renderer(iconSvg, *cache.resvgOptions);
    if (!renderer.isValid()) {
        qCritical(
            "Invalid SVG generated from slot %#x:\n%s", slot,
            iconSvg.toStdString().c_str());
        return QPixmap();
    }

    QImage icon = renderer.renderToImage(iconSize.toSize());
    QPixmap pixmap = QPixmap::fromImage(icon);
    cache.styleIcons.insert(slot, {info.genHash(), pixmap});
    return pixmap;
}

QByteArray Panel::genIconSvg(
    quint8 tSlot, quint8 rSlot, quint8 subSlot,
    const Config::ButtonInfo &info) const {
    using C::R30, C::R60, C::R45;
    namespace CGK = C::C::G::K;      // button config Keys
    namespace CBK = C::C::B::K;      // button config Keys
    namespace DIS = C::C::G::V::DIS; // default icon style

    Config::Slot slot = Config::calcSlot(pSlot, tSlot, rSlot, subSlot);
    Button *button = styleButtons[slot].get();
    QSizeF size = button->inactiveGeometry.size() * button->hoverScale;
    QPointF centroid = button->centroid * button->hoverScale;

    // true = pointing up, false = pointing down
    bool orientation = (tSlot + subSlot) % 2;

    // Return custom icon if defined
    if (info.userIconSvg.size() > 0)
        return info.userIconSvg;

    QString svgDefs;
    QString svgContent;

    // 0. Generate a indicator if this is a non-standard style
    {
        if (info.styles.empty()) {
            svgContent +=
                QString(
                    R"(<text x="%1" y="%2" fill="#fff" style="%3">?</text>)")
                    .arg(size.width() * 0.5)
                    .arg(size.height() * (orientation ? 0.5 : 0.85))
                    .arg(QString("font-size:%1;text-anchor:middle")
                             .arg(size.height() * 0.5));
        }
    }

    // 1.1 Calculate common anchor points for subsequent drawing
    auto has = [&](const char *k) -> bool { return info.styles.contains(k); };
    constexpr qreal strokeWidth = 5;       // Outer stroke width
    constexpr qreal checkerboardWidth = 5; // Checkerboard grid width
    // color/stroke-width/marker indicator's top/bottom-left/right point
    QPointF tr, bl, str, stl, mbl, mbr;
    // color/gradient indicator's radius
    qreal radius = size.height() / 3. - strokeWidth / 2.;
    // stroke-width/marker indicator's radius
    qreal sradius = size.height() * 11. / 24., mradius = sradius;
    // qreal mradius = size.height() * 3. / 8.;
    if (config->defaultIconStyle == DIS::circle) {
        tr = centroid + QPointF(radius * qCos(R60), -radius * qSin(R60));
        bl = centroid + QPointF(-radius * qCos(R60), radius * qSin(R60));
        qreal invert = orientation ? -1 : 1;
        str = centroid
              + QPointF(sradius * qCos(R30), -sradius * qSin(R30)) * invert;
        stl = centroid
              + QPointF(-sradius * qCos(R30), -sradius * qSin(R30)) * invert;
        mbl = centroid
              + QPointF(-mradius * qCos(R30), mradius * qSin(R30) * invert);
        mbr = centroid
              + QPointF(mradius * qCos(R30), mradius * qSin(R30) * invert);
    } else if (config->defaultIconStyle == DIS::square) {
        tr = centroid + QPointF(radius * qCos(R45), -radius * qSin(R45));
        bl = centroid + QPointF(-radius * qCos(R45), radius * qSin(R45));
        str = centroid + QPointF(sradius * qCos(R45), -sradius * qSin(R45));
        stl = centroid + QPointF(-sradius * qCos(R45), -sradius * qSin(R45));
        mbl = centroid + QPointF(-mradius * qCos(R30), mradius * qSin(R30));
        mbr = centroid + QPointF(mradius * qCos(R30), mradius * qSin(R30));
    };

    // 1.2 Add necessary definitions
    for (const QString &defId : info.defIds)
        if (config->svgDefs.contains(defId))
            svgDefs.append(config->svgDefs[defId]);
    // Replace non-displayable attributes so that markers can be displayed.
    QRegularExpression re(R"(\b(context-stroke|context-fill)\b)");
    svgDefs.replace(re, "#fff");

    // 2. Draw the fill/stroke color/style indicator
    if (has(CBK::fill) || has(CBK::stroke) || has(CBK::strokeDashArray)) {
        // Set geometry for the svg element
        QString element;
        if (config->defaultIconStyle == DIS::circle) {
            element =
                QString(
                    R"(<path d="M %1 %2 A %3 %3 0 0 0 %4 %5" style="{S}"/>)")
                    .arg(tr.x())
                    .arg(tr.y())
                    .arg(radius)
                    .arg(bl.x())
                    .arg(bl.y());
        } else if (config->defaultIconStyle == DIS::square) {
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
            styleString.append(QString("stroke-width:%1").arg(strokeWidth));
        for (const char *key : {CBK::fill, CBK::stroke, CBK::strokeDashArray})
            if (has(key))
                styleString.append(key + (":" + info.styles[key]));
        element.replace("{S}", styleString.join(";"));
        svgContent += element;
    }

    // 3. Draw the stroke/fill opacity indicator
    if (has(CBK::strokeOpacity) || has(CBK::fillOpacity)) {
        QString checkerboard =
            QString(
                R"(<pattern id="__checkerboard" patternUnits="userSpaceOnUse")"
                R"( width="%1" height="%1">)"
                R"(  <rect x="0" y="0" width="%2" height="%2" fill="#777"/>)"
                R"(  <rect x="0" y="%2" width="%2" height="%2" fill="#fff"/>)"
                R"(  <rect x="%2" y="%2" width="%2" height="%2" fill="#777"/>)"
                R"(  <rect x="%2" y="0" width="%2" height="%2" fill="#fff"/>)"
                R"(</pattern>)")
                .arg(2 * checkerboardWidth)
                .arg(checkerboardWidth);
        svgDefs.append(checkerboard);

        QString background, element;
        if (config->defaultIconStyle == DIS::circle) {
            background = element =
                QString(
                    R"(<path d="M %1 %2 A %3 %3 0 0 0 %4 %5" style="{S}"/>)")
                    .arg(bl.x())
                    .arg(bl.y())
                    .arg(radius)
                    .arg(tr.x())
                    .arg(tr.y());
        } else if (config->defaultIconStyle == DIS::square) {
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
            styleString.append(QString("stroke-width:%1").arg(strokeWidth));
            bgStyleString.append(QString("stroke-width:%1").arg(strokeWidth));
            bgStyleString.append("stroke:url(#__checkerboard)");
        }

        background.replace("{S}", bgStyleString.join(';'));
        svgContent += background;

        for (const char *key :
             {CBK::fill, CBK::stroke, CBK::stroke, CBK::fillOpacity,
              CBK::strokeOpacity})
            if (has(key))
                styleString.append(key + (":" + info.styles[key]));
        element.replace("{S}", styleString.join(';'));
        svgContent += element;
    }

    // 4. Draw stroke-width indicator
    if (has(CBK::strokeWidth)) {
        QString element;
        if (config->defaultIconStyle == DIS::circle) {
            element =
                QString(
                    R"(<path d="M %1 %2 A %3 %3 0 0 0 %4 %5" style="{S}"/>)")
                    .arg(str.x())
                    .arg(str.y())
                    .arg(sradius)
                    .arg(stl.x())
                    .arg(stl.y());
        } else if (config->defaultIconStyle == DIS::square) {
            element = QString(R"(<path d="M %1 %2 H %3" style="{S}"/>)")
                          .arg(str.x())
                          .arg(str.y())
                          .arg(stl.x())
                          .arg(stl.y());
        }

        QStringList styleString;
        styleString.append("fill-opacity:0");
        styleString.append(
            has(CBK::stroke)
                ? QString("stroke:%1").arg(info.styles[CBK::stroke])
                : "stroke:#fff");
        styleString.append(
            QString("stroke-width:%1").arg(info.styles[CBK::strokeWidth]));
        element.replace("{S}", styleString.join(';'));
        svgContent.append(element);
    }

    // 5. Draw marker start/end/mid indicator
    if (has(CBK::markerStart) || has(CBK::markerMid) || has(CBK::markerEnd)) {
        QString element = QString(R"(<path d="M %1 %2 H %3" style="{S}"/>)")
                              .arg(mbl.x())
                              .arg(mbl.y())
                              .arg(mbr.x());
        QStringList styleString;
        styleString.append("stroke-width:2");
        styleString.append(
            has(CBK::stroke)
                ? QString("stroke:%1").arg(info.styles[CBK::stroke])
                : "stroke:#fff");
        for (const char *key :
             {CBK::markerStart, CBK::markerEnd, CBK::markerMid})
            if (has(key))
                styleString.append(key + (":" + info.styles[key]));
        element.replace("{S}", styleString.join(';'));
        svgContent.append(element);
    }

    // 6. Draw font-style indicator
    if (has(CBK::fontFamily) || has(CBK::fontStyle)) {
        QString element =
            QString(R"(<text x="%1" y="%2" fill="#fff" style="{S}">%3</text>)")
                .arg(size.width() * 0.5)
                .arg(size.height() * (orientation ? 0.5 : 0.85))
                .arg(config->defaultIconText);

        QStringList styleString;

        // Set default fsize
        styleString.append(QString("font-size:%1").arg(size.height() * 0.5));
        // Correctly position the text
        styleString.append("text-anchor:middle");
        styleString.append("fill:#fff");

        for (const char *key : {CBK::fontFamily, CBK::fontStyle})
            if (has(key))
                styleString.append(key + (":" + info.styles[key]));
        element.replace("{S}", styleString.join(';'));
        svgContent += element;
    }

    // Compose final icon
    return QString(R"(<svg width="%1" height="%2" version="1.1")"
                   R"( viewBox="0 0 %1 %2" xmlns="http://www.w3.org/2000/svg">)"
                   R"( <defs>%3</defs>%4</svg>)")
        .arg(size.width())
        .arg(size.height())
        .arg(svgDefs)
        .arg(svgContent)
        .toUtf8();
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

Panel::Panel(Panel *parent, quint8 tSlot, const QSharedPointer<Config> &config)
    : QWidget(nullptr), config(parent ? parent->config : config),
      _pGrid(parent ? parent->_pGrid : QSharedPointer<PGrid>(new PGrid)),
      panelGrid(*_pGrid),
      coordinate(parent ? parent->calcRelativeCoordinate(tSlot) : QPoint{0, 0}),
      pSlot(parent ? parent->parentPanel ? parent->pSlot + 6 : tSlot + 1 : 0),
      parentPanel(parent), tSlot(tSlot), childPanels(6, nullptr),
      borderButtons(6, nullptr), hoverScale(1.5), unitLen(200), gapLen(3) {
    using C::R60;

    // Preconditions
    Q_ASSERT_X(this->config, __func__, "Config not initialized");
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
        for (quint8 j = 1; j <= 2; ++j)
            for (quint8 k = 0; k < j * 2 + 1; ++k)
                addStyleButton(i, j, k);

    // Add border buttons, skip the occupied edge
    for (quint8 i = 0; i < 6; ++i)
        if (!parentPanel || !panelGrid.contains(calcRelativeCoordinate(i)))
            addBorderButton(i);

    updateMask();
}

Panel::~Panel() {
    // unregister this from panelGrid
    panelGrid.remove(coordinate);
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
    Q_ASSERT(1 <= rSlot && rSlot <= 2);
    Q_ASSERT(subSlot <= rSlot * 2);

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
    QVector<QPointF> points = {
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

    QPointF centroid = std::reduce(points.begin(), points.end()) / 3.;
    QPolygonF mask(points);
    QRectF geometry(mask.boundingRect());
    mask.translate(-geometry.topLeft());

    // Add the button to styleButtons
    Config::Slot slot = Config::calcSlot(pSlot, tSlot, rSlot, subSlot);
    QSharedPointer<Button> button(
        new Button(
            geometry.toRect(), mask, hoverScale, centroid - geometry.topLeft(),
            geometry.topLeft() - geometry.toRect().topLeft(), this),
        [](Button *b) { b->deleteLater(); });
    styleButtons.insert(slot, button);

    // Draw icon on the button
    if (config->buttons.contains(slot)) {
        // Draw and set icon for this button
        button->setIcon(
            drawIcon(tSlot, rSlot, subSlot, *config->buttons[slot]));
        button->setIconSize(button->geometry().size());
    }
    button->show();

    // Raise on mouseEnter for better looking
    connect(button.get(), &Button::mouseEnter, this, &QWidget::raise);
    // Set the style on click
    connect(button.get(), &QPushButton::clicked, this, [this, slot] {
        copyStyle(slot);
    });

    return button.get();
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

    // Using delayed delete is necessary since it is possible that the deletion
    // is triggered during the button's own event
    QSharedPointer<HiddenButton> newButton(
        new HiddenButton(this), [](HiddenButton *b) { b->deleteLater(); });

    // Add button to UI
    borderButtons[tSlot] = newButton;
    newButton->setGeometry(geometry.toRect());
    newButton->setMask(mask.toPolygon());
    newButton->show();

    // Connect button functions
    connect(newButton.get(), &HiddenButton::mouseEnter, this, [this, tSlot] {
        Panel::addPanel(tSlot);
    });
    return borderButtons[tSlot].data();
}

void Panel::delBorderButton(quint8 tSlot) {
    borderButtons[tSlot] = nullptr;
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

void Panel::closeEvent(QCloseEvent *) {
    // Close all child panels first
    childPanels.fill(nullptr);

    // Restore border buttons of neighboring panels
    for (quint8 tSlot = 0; tSlot < 6; ++tSlot)
        if (QPoint coordinate = calcRelativeCoordinate(tSlot);
            panelGrid.contains(coordinate)) {
            Panel *panel = panelGrid[coordinate];
            panel->addBorderButton((tSlot + 3) % 6);
            panel->updateMask();
        }
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

void Panel::enterEvent(QEvent *) {
    // Close all child panels
    for (int tSlot = 0; tSlot < childPanels.size(); ++tSlot)
        if (childPanels[tSlot])
            childPanels[tSlot] = nullptr;
}

void Panel::addPanel(quint8 tSlot) {
    Q_ASSERT(tSlot <= 5);

    // Make sure the panel is only added once
    if (childPanels[tSlot])
        return;

    QSharedPointer<Panel> panel(new Panel(this, tSlot), [](Panel *panel) {
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

void Panel::copyStyle(Config::Slot slot) {
    if (config->buttons.contains(slot)) {
        // Copy style associated with slot to clipboard
        QMimeData *styleSvg = new QMimeData;
        styleSvg->setData(C::styleMimeType, config->buttons[slot]->styleSvg);
        QApplication::clipboard()->setMimeData(styleSvg);
        qDebug() << "Style copied " << styleSvg->data(C::styleMimeType);
    } else {
        qDebug() << "No style copied";
    }
}
