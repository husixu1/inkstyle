#include "panel.h"

#include "button.h"

#include <QCursor>
#include <QFile>
#include <QMoveEvent>
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

Panel::Panel(Panel *parent)
    : QWidget(nullptr), parentPanel(parent), childPanel(nullptr), unitLen(200),
      gapLen(2) {
    // setPalette(QPalette(QPalette::Window, Qt::transparent));
    // setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::X11BypassWindowManagerHint);
    setAttribute(Qt::WA_X11NetWmWindowTypePopupMenu);
    setWindowFlags(Qt::FramelessWindowHint);
    redrawMainWindow();
}

void Panel::redrawMainWindow() {
    // Set window location and size to be the bounding box of the hexagon
    QPoint cursorPos = QCursor::pos();
    if (parentPanel)
        position = parentPanel->position
                   + QPoint(
                       int(unitLen * qSqrt(3) * qCos(R30)),
                       int(unitLen * qSqrt(3) * qSin(R30)));
    else
        position = cursorPos - QPoint(unitLen, int(unitLen * qSin(R60)));

    setFixedSize(unitLen * 2, unitLen * qSin(R60) * 2);
    move(position);

    // Generate a hexagon
    QPolygon polygon;
    QVector<int> points;
    for (size_t i = 0; i < 6; ++i) {
        points.append(
            {static_cast<int>(unitLen + unitLen * qCos(R60 * i)),
             static_cast<int>(unitLen * qSin(R60) + unitLen * qSin(R60 * i))});
    }
    polygon.setPoints(6, points.data());

    // Set mask
    setMask(QRegion(polygon));

    // Add style buttons
    for (quint8 i = 0; i < 6; ++i) {
        for (quint8 j = 0; j < 2; ++j) {
            for (quint8 k = 0; k < (j + 1) * 2 + 1; ++k) {
                addStyleButton(i, j + 1, k);
            }
        }
    }

    // Add style buttons
    QPushButton *button;
    for (quint8 i = 0; i < 6; ++i) {
        button = addBorderButton(i);
        connect(button, SIGNAL(mouseEnter()), this, SLOT(addPanel()));
    }

    // Connect button slots
}

QPushButton *Panel::addStyleButton(quint8 tSlot, quint8 rSlot, quint8 subSlot) {
    QPushButton *button = new QPushButton(this);

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
            int(unitLen
                // Base x
                + ((rSlot * qCos(tSlot * R60)
                    + (subSlot / 2) * qCos((tSlot + 2) * R60))
                       * unitLen / 3.
                   // Offset for border
                   + gapLen * qCos((tSlot + 0.5 + (subSlot % 2)) * R60))),
            int(unitLen * qSin(R60)
                // Base y
                - ((rSlot * qSin(tSlot * R60)
                    + (subSlot / 2) * qSin((tSlot + 2) * R60))
                       * unitLen / 3.
                   // Offset for border
                   + gapLen * qSin((tSlot + 0.5 + (subSlot % 2)) * R60))),
        },
        {
            // The 2nd point
            int(unitLen
                + (((rSlot + 1 - (subSlot % 2)) * qCos(tSlot * R60)
                    + ((subSlot + 1) / 2) * qCos((tSlot + 2) * R60))
                       * unitLen / 3.
                   + gapLen * qCos((tSlot + 2.5 - (subSlot % 2) * 3) * R60))),
            int(unitLen * qSin(R60)
                - (((rSlot + 1 - (subSlot % 2)) * qSin(tSlot * R60)
                    + ((subSlot + 1) / 2) * qSin((tSlot + 2) * R60))
                       * unitLen / 3.
                   + gapLen * qSin((tSlot + 2.5 - (subSlot % 2) * 3) * R60))),
        },
        {
            // The 3rd point
            int(unitLen
                + (((rSlot + 1) * qCos(tSlot * R60)
                    + (subSlot / 2 + 1) * qCos((tSlot + 2) * R60))
                       * unitLen / 3.
                   + gapLen * qCos((tSlot - 1.5 - (subSlot % 2)) * R60))),
            int(unitLen * qSin(R60)
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

    button->setGeometry(minX, minY, maxX - minX, maxY - minY);
    button->setFixedSize(maxX - minX, maxY - minY);
    button->setText("T");

    QPolygon polygon;
    polygon.setPoints(
        3, points[0].x() - minX, points[0].y() - minY, points[1].x() - minX,
        points[1].y() - minY, points[2].x() - minX, points[2].y() - minY);

    // Set mask
    button->setMask(QRegion(polygon));

    return button;
}

QPushButton *Panel::addBorderButton(quint8 tSlot) {
    Button *button = new Button(this);
    assert(tSlot <= 5);

    // vertex of the trapezoidal button
    QVector<QPoint> points = {
        {
            int(unitLen + qCos(tSlot * R60) * unitLen),
            int(unitLen * qSin(R60) - (qSin(tSlot * R60) * unitLen)),
        },
        {
            int(unitLen + qCos((tSlot + 1) * R60) * unitLen),
            int(unitLen * qSin(R60) - (qSin((tSlot + 1) * R60) * unitLen)),
        },
        {
            int(unitLen + qCos((tSlot + 1) * R60) * (unitLen - gapLen)),
            int(unitLen * qSin(R60)
                - ((qSin((tSlot + 1) * R60)) * (unitLen - gapLen))),
        },
        {
            int(unitLen + qCos(tSlot * R60) * (unitLen - gapLen)),
            int(unitLen * qSin(R60) - (qSin(tSlot * R60) * (unitLen - gapLen))),
        }};

    QVector<int> xs = {
        points[0].x(), points[1].x(), points[2].x(), points[3].x()};
    QVector<int> ys = {
        points[0].y(), points[1].y(), points[2].y(), points[3].y()};

    int minX = *std::min_element(xs.begin(), xs.end());
    int minY = *std::min_element(ys.begin(), ys.end());
    int maxX = *std::max_element(xs.begin(), xs.end());
    int maxY = *std::max_element(ys.begin(), ys.end());

    button->setGeometry(minX, minY, maxX - minX, maxY - minY);
    button->setFixedSize(maxX - minX, maxY - minY);
    // button->setText("");

    QPolygon polygon;
    polygon.setPoints(
        4, points[0].x() - minX, points[0].y() - minY, points[1].x() - minX,
        points[1].y() - minY, points[2].x() - minX, points[2].y() - minY,
        points[3].x() - minX, points[3].y() - minY);

    // Set mask
    button->setMask(QRegion(polygon));

    return button;
}

void Panel::moveEvent(QMoveEvent *event) {
    this->position = event->pos();
    if (this->childPanel)
        childPanel->move(childPanel->position + event->pos() - event->oldPos());
}

void Panel::addPanel() {
    if (!childPanel) {
        std::cout << "add panel" << std::endl;
        childPanel = QSharedPointer<Panel>(
            new Panel(this), [](Panel *panel) { panel->close(); });
        childPanel->show();
        childPanel->move(childPanel->position);
    }
}
