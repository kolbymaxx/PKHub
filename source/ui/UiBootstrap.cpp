#include "pkhub/ui/UiBootstrap.hpp"

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
#include "pkhub/ui/views/BoxGridView.hpp"
#include "pkhub/ui/views/PokemonSlotView.hpp"
#endif

namespace pkhub::ui {

void registerCustomViews() {
#if defined(PKHUB_HAS_BOREALIS)
    // Pokémon HOME / PC companion: deep teal ground, mint accent — not purple/glow.
    auto& dark = brls::Theme::getDarkTheme();
    dark.addColor("brls/background", nvgRGB(8, 14, 18));
    dark.addColor("brls/text", nvgRGB(236, 246, 240));
    dark.addColor("brls/text_disabled", nvgRGB(90, 118, 108));
    dark.addColor("brls/accent", nvgRGB(72, 186, 154));
    dark.addColor("brls/highlight/color1", nvgRGB(72, 186, 154));
    dark.addColor("brls/highlight/color2", nvgRGB(40, 120, 100));
    dark.addColor("brls/list/listItem_value_color", nvgRGB(150, 198, 176));
    dark.addColor("brls/header/border", nvgRGBA(72, 186, 154, 110));
    dark.addColor("brls/sidebar/background", nvgRGB(12, 20, 24));
    dark.addColor("brls/sidebar/active_item", nvgRGB(72, 186, 154));
    dark.addColor("brls/sidebar/separator", nvgRGBA(72, 186, 154, 55));
    dark.addColor("brls/applet_frame/separator", nvgRGBA(72, 186, 154, 80));
    dark.addColor("brls/button/primary_enabled_background", nvgRGB(40, 118, 98));
    dark.addColor("brls/button/primary_enabled_text", nvgRGB(235, 248, 242));
    dark.addColor("brls/button/primary_disabled_background", nvgRGB(28, 40, 36));

    auto& light = brls::Theme::getLightTheme();
    light.addColor("brls/accent", nvgRGB(24, 118, 94));
    light.addColor("brls/background", nvgRGB(232, 242, 238));

    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);

    brls::Application::registerXMLView("PokemonSlotView", PokemonSlotView::create);
    brls::Application::registerXMLView("BoxGridView", BoxGridView::create);
#endif
}

}  // namespace pkhub::ui
