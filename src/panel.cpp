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
#include <QSvgRenderer>
#include <QVector>
#include <QtDebug>
#include <QtMath>
#include <algorithm>

static constexpr double R30 = 30. * M_PI / 180.;
static constexpr double R60 = 60. * M_PI / 180.;

QHash<QPoint, Panel *> Panel::panelGrid;
uint qHash(const QPoint &point, uint seed = 0) {
    return qHash(QPair<int, int>(point.x(), point.y()), seed);
}

ConfigManager *Panel::config = nullptr;
void Panel::setConfig(ConfigManager *newConfig) {
    config = newConfig;
}

QPixmap Panel::drawIcon(
    const QSizeF &iconSize, const QPointF &centroid,
    const ConfigManager::ButtonInfo &button) {

    QPixmap pixmap(iconSize.toSize());
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setRenderHints(
        QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

    // Move the icon's centroid to the button's centroid
    constexpr float S = 2. / 3.;
    QPointF scaledCentroid(
        iconSize.width() * S / 2, iconSize.height() * S / 2.);
    painter.setWorldTransform(
        QTransform::fromScale(S, S)
        * QTransform::fromTranslate(
            (centroid - scaledCentroid).x(), (centroid - scaledCentroid).y()));

    // Render the button's svg
    QSvgRenderer renderer(button.genIconSvg(iconSize));
    renderer.setAspectRatioMode(Qt::KeepAspectRatio);
    renderer.render(&painter);

    return pixmap;
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
    return pos()
           + QPointF(
                 unitLen * qSqrt(3) * qCos(R30 + R60 * tSlot),
                 -unitLen * qSqrt(3) * qSin(R30 + R60 * tSlot))
                 .toPoint();
}

QVector<QPoint> Panel::genBorderButtonMask(quint8 tSlot) {
    return {
        {
            int(size().width() / 2. + qCos(tSlot * R60) * unitLen),
            int(size().height() / 2. - (qSin(tSlot * R60) * unitLen)),
        },
        {
            int(size().width() / 2. + qCos((tSlot + 1) * R60) * unitLen),
            int(size().height() / 2. - (qSin((tSlot + 1) * R60) * unitLen)),
        },
        {
            int(size().width() / 2.
                + (qCos((tSlot + 1) * R60) * 4. + qCos((tSlot - 1) * R60))
                      * unitLen / 3.),
            int(size().height() / 2.
                - (qSin((tSlot + 1) * R60) * 4. + qSin((tSlot - 1) * R60))
                      * unitLen / 3.),
        },
        {
            int(size().width() / 2.
                + (qCos(tSlot * R60) * 4. + qCos((tSlot + 2) * R60)) * unitLen
                      / 3.),
            int(size().height() / 2.
                - (qSin(tSlot * R60) * 4. + qSin((tSlot + 2) * R60)) * unitLen
                      / 3.),
        }};
}

Panel::Panel(Panel *parent, quint8 tSlot)
    : QWidget(nullptr),
      coordinate(parent ? parent->calcRelativeCoordinate(tSlot) : QPoint{0, 0}),
      pSlot(parent ? parent->parentPanel ? parent->pSlot + 6 : tSlot + 1 : 0),
      parentPanel(parent), tSlot(tSlot), childPanels(6, nullptr),
      borderButtons(6, nullptr), hoverScale(1.5), unitLen(200), gapLen(3) {
    // Preconditions
    Q_ASSERT_X(config, __func__, "Global config not initialized");

    // Set common window attributes
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlag(Qt::FramelessWindowHint);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowFlag(Qt::NoDropShadowWindowHint);

    // Add panel to grid
    panelGrid[coordinate] = this;

    // Set window location and size to be the bounding box of the hexagon
    setFixedSize(
        int(unitLen * (2 + 2 / 3.)), unitLen * qSin(R60) * (2 + 2 / 3.));

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
            mask += QRegion(QPolygon(genBorderButtonMask(tSlot)));

    // Set new mask
    clearMask();
    setMask(mask);
}

Button *Panel::addStyleButton(quint8 tSlot, quint8 rSlot, quint8 subSlot) {
    Q_ASSERT(tSlot <= 5);
    Q_ASSERT(1 <= rSlot && rSlot <= 2);
    Q_ASSERT(subSlot <= rSlot * 2);

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

    QVector<qreal> xs = {points[0].x(), points[1].x(), points[2].x()};
    QVector<qreal> ys = {points[0].y(), points[1].y(), points[2].y()};

    qreal minX = *std::min_element(xs.begin(), xs.end());
    qreal minY = *std::min_element(ys.begin(), ys.end());
    qreal maxX = *std::max_element(xs.begin(), xs.end());
    qreal maxY = *std::max_element(ys.begin(), ys.end());
    QPointF centroid = std::reduce(points.begin(), points.end()) / 3.;

    QPolygonF mask(points);
    mask.translate(-minX, -minY);

    QRectF geometry(minX, minY, maxX - minX, maxY - minY);
    Button *button = new Button(
        geometry.toRect(), mask, hoverScale, centroid - geometry.topLeft(),
        geometry.topLeft() - geometry.toRect().topLeft(), this);

    // Draw icon on the button
    if (ConfigManager::Slot slot =
            ConfigManager::calcSlot(pSlot, tSlot, rSlot, subSlot);
        config->buttons.contains(slot)) {

        QSizeF iconSize(
            int(unitLen / 3. - 2 * qCos(R30) * gapLen),
            int(qSin(R60) * (unitLen / 3. - 2 * qCos(R30) * gapLen)));

        // Draw with 2x size, otherwise icon won't scale
        button->setIcon(drawIcon(
            iconSize * hoverScale, (centroid - geometry.topLeft()) * hoverScale,
            *config->buttons[slot]));

        // button->setIcon(QIcon(C::SC::circleIcon));
        button->setIconSize(iconSize.toSize());
    }
    button->show();

    // Raise on mouseEnter for better looking
    connect(button, &Button::mouseEnter, this, &QWidget::raise);
    // Set the style on click
    connect(button, &QPushButton::clicked, this, [this, tSlot, rSlot, subSlot] {
        copyStyle(ConfigManager::calcSlot(pSlot, tSlot, rSlot, subSlot));
    });

    return button;
}

HiddenButton *Panel::addBorderButton(quint8 tSlot) {
    Q_ASSERT(tSlot <= 5);

    if (borderButtons[tSlot])
        return nullptr;

    // vertex of the trapezoidal button
    QVector<QPoint> points = genBorderButtonMask(tSlot);

    QVector<int> xs = {
        points[0].x(), points[1].x(), points[2].x(), points[3].x()};
    QVector<int> ys = {
        points[0].y(), points[1].y(), points[2].y(), points[3].y()};

    int minX = *std::min_element(xs.begin(), xs.end());
    int minY = *std::min_element(ys.begin(), ys.end());
    int maxX = *std::max_element(xs.begin(), xs.end());
    int maxY = *std::max_element(ys.begin(), ys.end());

    QPolygon polygon;
    polygon.setPoints(
        4, points[0].x() - minX, points[0].y() - minY, points[1].x() - minX,
        points[1].y() - minY, points[2].x() - minX, points[2].y() - minY,
        points[3].x() - minX, points[3].y() - minY);
    QRegion mask(polygon);

    // Using delayed delete is necessary since it is possible that the deletion
    // is triggered during the button's own event
    QSharedPointer<HiddenButton> newButton(
        new HiddenButton(this), [](HiddenButton *b) { b->deleteLater(); });

    // Add button to UI
    borderButtons[tSlot] = newButton;
    newButton->setGeometry(QRect(minX, minY, maxX - minX, maxY - minY));
    newButton->setMask(mask);
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

void Panel::enterEvent(QEvent *event) {
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

void Panel::copyStyle(ConfigManager::Slot slot) {
    if (config->buttons.contains(slot)) {
        // Copy style associated with slot to clipboard
        QMimeData *styleSvg = new QMimeData;
        styleSvg->setData(
            C::ST::styleMimeType, config->buttons[slot]->styleSvg);
        QApplication::clipboard()->setMimeData(styleSvg);
        qDebug() << "Style copied " << styleSvg->data(C::ST::styleMimeType);
    } else {
        qDebug() << "No style copied";
    }
}
