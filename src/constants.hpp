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
    cccp svgDefs = "svg-defs";
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
            cccp markerStart = "marker-start";
            cccp markerMid = "marker-mid";
            cccp markerEnd = "marker-end";
            cccp fill = "fill";
            cccp fillOpacity = "fill-opacity";
            cccp fontFamily = "font-family";
            cccp fontSize = "font-size";
            cccp fontStyle = "font-style";
            /// @brief keys in this array will be automatically added to
            /// #Config::ButtonInfo::styles
            cccp basicStyles[] = {stroke,          strokeWidth, strokeOpacity,
                                  strokeDashArray, markerStart, markerMid,
                                  markerEnd,       fill,        fillOpacity,
                                  fontFamily,      fontSize,    fontStyle};
            cccp slot = "slot";
            cccp customStyle = "svg";
            cccp customIcon = "icon";
        } // namespace Keys
        namespace K = Keys;
    } // namespace Button
    namespace B = Button;
    namespace SvgDefs {
        namespace Keys {
            cccp id = "id";
            cccp type = "type";
            cccp attrs = "attrs";
            cccp svg = "svg";
        } // namespace Keys
        namespace K = Keys;
    } // namespace SvgDefs
    namespace SD = SvgDefs;
} // namespace Configs
namespace C = Configs;

} // namespace Constants
namespace C = Constants;

#endif // CONSTANTS_HPP
