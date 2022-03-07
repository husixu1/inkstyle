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
namespace G = GlobalConfigKeys;

constexpr const char *buttonsConfig = "buttons";
namespace ButtonConfigKeys {
constexpr const char *slot = "slot";
}
namespace B = ButtonConfigKeys;

} // namespace ConfigKeys
namespace CK = ConfigKeys;

namespace StyleTemplate {
constexpr const char *plainStyleTemplate =
    "(<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
    "(<svg><inkscape:clipboard style=\"{style_string}\"/></svg>";
} // namespace StyleTemplate
namespace ST = StyleTemplate;

} // namespace Constants
namespace C = Constants;

#endif // CONSTANTS_HPP
