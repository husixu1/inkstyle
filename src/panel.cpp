#include "panel.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QFile>
#include <QMoveEvent>
#include <QPaintEngine>
#include <QPainter>
#include <QPalette>
#include <QPolygon>
#include <QPushButton>
#include <QRegion>
#include <QVector>
#include <QtMath>
#include <algorithm>
#include <iostream>

static constexpr double R30 = 30. * M_PI / 180.;
static constexpr double R60 = 60. * M_PI / 180.;

QPoint Panel::calcRelativePanelPos(quint8 tSlot) {
    return pos()
           + QPoint(
               int(unitLen * qSqrt(3) * qCos(R30 + R60 * tSlot)),
               int(-unitLen * qSqrt(3) * qSin(R30 + R60 * tSlot)));
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
    : QWidget(nullptr), parentPanel(parent), tSlot(tSlot),
      childPanels(6, nullptr), borderButtons(6, nullptr), unitLen(200),
      gapLen(2) {
    // setPalette(QPalette(QPalette::Window, Qt::transparent));
    setAttribute(Qt::WA_TranslucentBackground);
    // setWindowFlags(Qt::X11BypassWindowManagerHint);
    // setAttribute(Qt::WA_X11NetWmWindowTypePopupMenu);
    setWindowFlags(Qt::FramelessWindowHint);
    redraw();
}

void Panel::redraw() {
    // Set window location and size to be the bounding box of the hexagon
    setFixedSize(
        int(unitLen * (2 + 2 / 3.)), unitLen * qSin(R60) * (2 + 2 / 3.));

    if (parentPanel)
        move(parentPanel->calcRelativePanelPos(tSlot));
    else
        move(QCursor::pos() - QPoint(unitLen, int(unitLen * qSin(R60))));

    // Add style buttons
    Button *button;
    for (quint8 i = 0; i < 6; ++i) {
        for (quint8 j = 0; j < 2; ++j) {
            for (quint8 k = 0; k < (j + 1) * 2 + 1; ++k) {
                button = addStyleButton(i, j + 1, k);
                connect(button, &QPushButton::clicked, this, &Panel::copyStyle);
            }
        }
    }

    // Add border buttons, skip the occupied edge
    for (quint8 i = 0; i < 6; ++i) {
        if (!parentPanel || (abs(i - tSlot) != 3)) {
            HiddenButton *borderButton = addBorderButton(i);
            connect(borderButton, &HiddenButton::mouseEnter, this, [=]() {
                Panel::addPanel(i);
            });
        }
    }

    updateMask();
}

void Panel::updateMask() {
    // Generate the center hexagon
    QPolygon polygon;
    QVector<int> points;
    for (size_t i = 0; i < 6; ++i) {
        points.append(
            {int(size().width() / 2. + unitLen * qCos(R60 * i)),
             int(size().height() / 2. + unitLen * qSin(R60 * i))});
    }
    polygon.setPoints(6, points.data());
    QRegion mask(polygon);

    // add border-button masks
    for (int tSlot = 0; tSlot < borderButtons.size(); ++tSlot) {
        if (borderButtons[tSlot]) {
            QVector<QPoint> points = genBorderButtonMask(tSlot);
            mask += QRegion(QPolygon(points));
        }
    }

    // Set mask
    setMask(mask);
}

Button *Panel::addStyleButton(quint8 tSlot, quint8 rSlot, quint8 subSlot) {
    assert(tSlot <= 5);
    assert(1 <= rSlot && rSlot <= 2);
    assert(subSlot <= rSlot * 2);

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
    // •: tranangle vertecies
    // 1: the 1st point
    // 2: the 2nd point
    // 3: the 3rd point

    // vertex of the triangular button
    QVector<QPoint> points = {
        {
            // The 1st point
            int(size().width() / 2.
                // Base x
                + ((rSlot * qCos(tSlot * R60)
                    + (subSlot / 2) * qCos((tSlot + 2) * R60))
                       * unitLen / 3.
                   // Offset for border
                   + gapLen * qCos((tSlot + 0.5 + (subSlot % 2)) * R60))),
            int(size().height() / 2.
                // Base y
                - ((rSlot * qSin(tSlot * R60)
                    + (subSlot / 2) * qSin((tSlot + 2) * R60))
                       * unitLen / 3.
                   // Offset for border
                   + gapLen * qSin((tSlot + 0.5 + (subSlot % 2)) * R60))),
        },
        {
            // The 2nd point
            int(size().width() / 2.
                + (((rSlot + 1 - (subSlot % 2)) * qCos(tSlot * R60)
                    + ((subSlot + 1) / 2) * qCos((tSlot + 2) * R60))
                       * unitLen / 3.
                   + gapLen * qCos((tSlot + 2.5 - (subSlot % 2) * 3) * R60))),
            int(size().height() / 2.
                - (((rSlot + 1 - (subSlot % 2)) * qSin(tSlot * R60)
                    + ((subSlot + 1) / 2) * qSin((tSlot + 2) * R60))
                       * unitLen / 3.
                   + gapLen * qSin((tSlot + 2.5 - (subSlot % 2) * 3) * R60))),
        },
        {
            // The 3rd point
            int(size().width() / 2.
                + (((rSlot + 1) * qCos(tSlot * R60)
                    + (subSlot / 2 + 1) * qCos((tSlot + 2) * R60))
                       * unitLen / 3.
                   + gapLen * qCos((tSlot - 1.5 - (subSlot % 2)) * R60))),
            int(size().height() / 2.
                - (((rSlot + 1) * qSin(tSlot * R60)
                    + (subSlot / 2 + 1) * qSin((tSlot + 2) * R60))
                       * unitLen / 3.
                   + gapLen * qSin((tSlot - 1.5 - (subSlot % 2)) * R60))),
        }};

    QVector<int> xs = {points[0].x(), points[1].x(), points[2].x()};
    QVector<int> ys = {points[0].y(), points[1].y(), points[2].y()};

    int minX = *std::min_element(xs.begin(), xs.end());
    int minY = *std::min_element(ys.begin(), ys.end());
    int maxX = *std::max_element(xs.begin(), xs.end());
    int maxY = *std::max_element(ys.begin(), ys.end());

    QPolygon mask;
    mask.setPoints(
        3, points[0].x() - minX, points[0].y() - minY, points[1].x() - minX,
        points[1].y() - minY, points[2].x() - minX, points[2].y() - minY);

    Button *button =
        new Button(QRect(minX, minY, maxX - minX, maxY - minY), mask, this);

    // TEST: Draw an icon
    QSize iconSize(
        int(unitLen / 3. - 2 * qSin(R60) * gapLen),
        int(qSin(R60) * unitLen / 3. - gapLen));

    QPixmap pixmap(iconSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHints(
        QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
    painter.setPen(Qt::white);

    // Translate and rotate are dependent. I.e. the coordinate system used
    // is always the painter's global coordinate system after movement.
    if (tSlot == 0 || tSlot == 3) {
        if ((subSlot + tSlot) % 2) {
            painter.rotate(60);
            painter.translate(0., -qSin(R60) * iconSize.width());
        } else {
            painter.translate(iconSize.width() / 2., 0);
            painter.rotate(60);
        }
    } else if (tSlot == 2 || tSlot == 5) {
        if ((subSlot + tSlot) % 2) {
            painter.rotate(-60);
            painter.translate(-iconSize.width() / 2., 0);
        } else {
            painter.translate(0., qSin(R60) * iconSize.width());
            painter.rotate(-60);
        }
    }

    painter.drawRect(
        iconSize.width() / 2. - 5, iconSize.height() - 10 - 1, 10, 10);
    button->setIcon(pixmap);
    button->setIconSize(iconSize);

    return button;
}

HiddenButton *Panel::addBorderButton(quint8 tSlot) {
    assert(tSlot <= 5);

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

    // button->setText("");

    QPolygon polygon;
    polygon.setPoints(
        4, points[0].x() - minX, points[0].y() - minY, points[1].x() - minX,
        points[1].y() - minY, points[2].x() - minX, points[2].y() - minY,
        points[3].x() - minX, points[3].y() - minY);
    QRegion mask(polygon);

    // Using delayed delete is necessary since it is possible that the deletion
    // is triggered during the button's own event
    borderButtons[tSlot] = QSharedPointer<HiddenButton>(
        new HiddenButton(this), [](HiddenButton *b) { b->deleteLater(); });
    borderButtons[tSlot]->setGeometry(
        QRect(minX, minY, maxX - minX, maxY - minY));
    borderButtons[tSlot]->setMask(mask);

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
    // close all child panels first
    for (auto &panel : childPanels)
        panel = nullptr;

    // close all parents
    if (parentPanel)
        parentPanel->close();
}

void Panel::addPanel(quint8 tSlot) {
    assert(0 <= tSlot && tSlot <= 5);
    delBorderButton(tSlot);
    updateMask();
    if (!childPanels[tSlot]) {
        std::cout << "add panel" << std::endl;
        childPanels[tSlot] = QSharedPointer<Panel>(
            new Panel(this, tSlot), [](Panel *panel) { panel->close(); });
        childPanels[tSlot]->show();
        childPanels[tSlot]->move(childPanels[tSlot]->pos());
    }
}

void Panel::delPanel(quint8 tSlot) {
    assert(0 <= tSlot && tSlot <= 5);
    if (!childPanels[tSlot])
        return;
    childPanels[tSlot] = nullptr;
}

void Panel::copyStyle() {
    QApplication::clipboard()->setText("some text");
    std::cout << "Text copied" << std::endl;
}
