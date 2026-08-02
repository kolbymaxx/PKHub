#include "pkhub/ui/UiBootstrap.hpp"

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
#include "pkhub/ui/views/BoxGridView.hpp"
#include "pkhub/ui/views/PokemonSlotView.hpp"
#endif

namespace pkhub::ui {

void registerCustomViews() {
#if defined(PKHUB_HAS_BOREALIS)
    // Forest-night beta look: deep teal ground, mint accent — not purple/glow.
    auto& dark = brls::Theme::getDarkTheme();
    dark.addColor("brls/background", nvgRGB(10, 16, 20));
    dark.addColor("brls/text", nvgRGB(232, 242, 236));
    dark.addColor("brls/text_disabled", nvgRGB(100, 122, 112));
    dark.addColor("brls/accent", nvgRGB(78, 176, 148));
    dark.addColor("brls/highlight/color1", nvgRGB(78, 176, 148));
    dark.addColor("brls/highlight/color2", nvgRGB(48, 120, 100));
    dark.addColor("brls/list/listItem_value_color", nvgRGB(150, 198, 176));
    dark.addColor("brls/header/border", nvgRGBA(78, 176, 148, 100));
    dark.addColor("brls/sidebar/background", nvgRGB(14, 22, 26));
    dark.addColor("brls/sidebar/active_item", nvgRGB(78, 176, 148));
    dark.addColor("brls/sidebar/separator", nvgRGBA(78, 176, 148, 50));
    dark.addColor("brls/applet_frame/separator", nvgRGBA(78, 176, 148, 80));
    dark.addColor("brls/button/primary_enabled_background", nvgRGB(48, 120, 100));
    dark.addColor("brls/button/primary_enabled_text", nvgRGB(235, 248, 242));

    auto& light = brls::Theme::getLightTheme();
    light.addColor("brls/accent", nvgRGB(28, 118, 94));
    light.addColor("brls/background", nvgRGB(236, 244, 240));

    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);

    brls::Application::registerXMLView("PokemonSlotView", PokemonSlotView::create);
    brls::Application::registerXMLView("BoxGridView", BoxGridView::create);
#endif
}

}  // namespace pkhub::ui
