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
consteval qreal RAD(qreal deg) noexcept {
    return deg * M_PI / 180;
}

/// @brief Number of icons to cache
constexpr size_t iconCacheSize = 1000;

/// @brief MIME type to be used by the clipboard
cccp styleMimeType = "image/x-inkscape-svg";

/// @brief Icon drawing-related constants
namespace IconDrawing {
    /// @brief Checkerboard grid width
    constexpr qreal checkerboardWidth = 5;
    /// @brief Stroke width of the circle when the 'stroke' attribute exists
    constexpr qreal colorStrokeWidth = 5;
    /// @brief Stroke width of the circle when the 'stroke' attribute is missing
    constexpr qreal otherStrokeWidth = 2;
} // namespace IconDrawing
namespace IC = IconDrawing;

/// @brief Default button colors when no config file is provided
namespace DefaultButtonColors {
    static constexpr QColor off(0x20, 0x20, 0x20, 0x80);
    static constexpr QColor on(0x10, 0x10, 0x10, 0x90);
}; // namespace DefaultButtonColors
namespace DBC = DefaultButtonColors;

namespace Configs {
    cccp global = "global";
    cccp buttons = "buttons";
    cccp svgDefs = "svg-defs";
    namespace Global {
        namespace Keys {
            cccp shortcutMainPanel = "shortcut-main-panel";
            cccp shortcutTex = "shortcut-tex";
            cccp shortcutCompiledTex = "shortcut-compiled-tex";
            cccp buttonBgColorInactive = "button-background-inactive";
            cccp buttonBgColorActive = "button-background-active";
            cccp guideColor = "guide-color";
            cccp panelMaxLevels = "panel-max-levels";
            cccp panelRadius = "panel-radius";
            cccp defaultIconStyle = "default-icon-style";
            cccp defaultIconText = "default-icon-text";
            cccp texCompileTemplate = "tex-compile-template";
            cccp texEditorCmd = "tex-editor-cmd";
            cccp texCompileCmd = "tex-compile-cmd";
            cccp pdfToSvgCmd = "pdf-to-svg-cmd";
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
            cccp strokeDashOffset = "stroke-dashoffset";
            cccp strokeLineCap = "stroke-linecap";
            cccp strokeLineJoin = "stroke-linejoin";
            cccp strokeMiterLimit = "stroke-miterlimit";
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
            cccp basicStyles[] = {
                stroke,          strokeOpacity,    strokeWidth,
                strokeDashArray, strokeDashOffset, strokeLineCap,
                strokeLineJoin,  strokeMiterLimit, markerStart,
                markerMid,       markerEnd,        fill,
                fillOpacity,     fontFamily,       fontSize,
                fontStyle};
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
