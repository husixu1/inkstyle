#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <QColor>
#include <QString>
#include <cmath>
#include <string>

/// @brief USSR?
#define cccp constexpr const char *

/// @brief Constants used throughout the program
namespace Constants {

constexpr double R30 = 30. * M_PI / 180.;
constexpr double R45 = 45. * M_PI / 180.;
constexpr double R60 = 60. * M_PI / 180.;

cccp styleMimeType = "image/x-inkscape-svg";

/// @brief Default button colors when no config file is provided
namespace DefaultButtonColors {
    static constexpr QColor off(0x20, 0x20, 0x20, 0x80);
    static constexpr QColor on(0x10, 0x10, 0x10, 0x90);
}; // namespace DefaultButtonColors
namespace DBC = DefaultButtonColors;

namespace Configs {
    // NOTE: const std::string are supported in gcc 12
    // current (Mar 2022) gcc 12 are still experimental
    cccp global = "global";
    cccp buttons = "buttons";
    namespace Global {
        namespace Keys {
            cccp panelBgColor = "panel-background";
            cccp buttonBgColorInactive = "button-background-inactive";
            cccp buttonBgColorActive = "button-background-active";
            cccp guideColor = "guide-color";
            cccp panelMaxLevels = "panel-max-levels";
            cccp panelRadius = "panel-radius";
            cccp defaultIconStyle = "default-icon-style";
            cccp defaultIconText = "default-icon-text";
        } // namespace Keys
        namespace K = Keys;
        namespace Values {
            namespace DefaultIconStyle {
                cccp circle = "circle";
                cccp square = "square";
            } // namespace DefaultIconStyle
            namespace DIS = DefaultIconStyle;
        } // namespace Values
        namespace V = Values;
    } // namespace Global
    namespace G = Global;
    namespace Button {
        namespace Keys {
            cccp stroke = "stroke";
            cccp strokeOpacity = "stroke-opacity";
            cccp strokeWidth = "stroke-width";
            cccp strokeDashArray = "stroke-dasharray";
            cccp strokeStart = "stroke-start";
            cccp strokeEnd = "stroke-end";
            cccp fill = "fill";
            cccp fillOpacity = "fill-opacity";
            cccp fillGradient = "fill-gradient";
            // non-standard stuff
            cccp fillMesh = "fill-mesh";
            cccp fontFamily = "font-family";
            cccp fontSize = "font-size";
            cccp fontStyle = "font-style";
            cccp basicStyles[] = {
                stroke,       strokeWidth, strokeOpacity, strokeDashArray,
                strokeStart,  strokeEnd,   fill,          fillOpacity,
                fillGradient, fillMesh,    fontFamily,    fontSize,
                fontStyle};
            cccp slot = "slot";
            cccp customStyle = "svg";
            cccp customIcon = "icon";
        } // namespace Keys
        namespace K = Keys;
    } // namespace Button
    namespace B = Button;
} // namespace Configs
namespace C = Configs;

} // namespace Constants
namespace C = Constants;

#endif // CONSTANTS_HPP
