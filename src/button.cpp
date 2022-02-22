#include "button.h"

Button::Button() {}

void Button::redraw() {
    //    constexpr double R60 = 60. * M_PI / 180.;
    //
    //    // Set window location and size to be the bounding box of the hexagon
    //    setFixedSize(unitLen * 2, unitLen * qSin(R60) * 2);
    //
    //    // Generate a hexagon
    //    QPolygon polygon;
    //    QVector<int> points;
    //    for (size_t i = 0; i < 6; ++i) {
    //        points.append(
    //            {static_cast<int>(unitLen + unitLen * qCos(R60 * i)),
    //             static_cast<int>(unitLen * qSin(R60) + unitLen * qSin(R60 *
    //             i))});
    //    }
    //    polygon.setPoints(6, points.data());
    //
    //    // Set mask
    //    setMask(QRegion(polygon));
}
