#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <QString>

namespace Constants {
namespace ConfigKeys {

constexpr const char *globalConfig = "global";
namespace GlobalConfigKeys {
constexpr const char *panelBgColor = "panel-background";
constexpr const char *buttonBgColor = "button-background";
constexpr const char *guideColor = "guide-color";
constexpr const char *panelMaxLevels = "panel-max-levels";
constexpr const char *panelRadius = "panel-radius";
} // namespace GlobalConfigKeys
namespace GK = GlobalConfigKeys;

constexpr const char *buttonsConfig = "buttons";
namespace ButtonConfigKeys {
constexpr const char *slot = "slot";
constexpr const char *strokeColor = "stroke";
constexpr const char *strokeWidth = "stroke-width";
constexpr const char *strokeDashArray = "stroke-dasharray";
constexpr const char *strokeStart = "stroke-start";
constexpr const char *strokeEnd = "stroke-end";
constexpr const char *fill = "fill";
constexpr const char *fillOpacity = "fill-opacity";
constexpr const char *fillGradient = "fill-gradient";
// non-standard stuff
constexpr const char *fillMesh = "fill-mesh";
constexpr const char *textFont = "text-font";
constexpr const char *textSize = "text-size";
constexpr const char *textStyle = "text-style";
constexpr const char *customSvg = "svg";
} // namespace ButtonConfigKeys
namespace BK = ButtonConfigKeys;

namespace ButtonTypeValues {} // namespace ButtonTypeValues
namespace BV = ButtonTypeValues;

} // namespace ConfigKeys
namespace CK = ConfigKeys;

namespace StyleConstants {
constexpr const char *plainStyleTemplate =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
    "<svg><inkscape:clipboard style=\"{style_string}\"/></svg>";
constexpr const char *stylePlaceHolder = "{style_string}";
constexpr const char *styleMimeType = "image/x-inkscape-svg";
} // namespace StyleConstants
namespace SC = StyleConstants;

} // namespace Constants
namespace C = Constants;

#endif // CONSTANTS_HPP
