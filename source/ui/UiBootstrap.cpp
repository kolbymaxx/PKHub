#include "pkhub/ui/UiBootstrap.hpp"

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
#include "pkhub/ui/views/BoxGridView.hpp"
#include "pkhub/ui/views/PokemonSlotView.hpp"
#endif

namespace pkhub::ui {

void registerCustomViews() {
#if defined(PKHUB_HAS_BOREALIS)
    // Soft teal accent over a deep forest night background — avoid purple/glow defaults.
    auto& dark = brls::Theme::getDarkTheme();
    dark.addColor("brls/background", nvgRGB(12, 18, 22));
    dark.addColor("brls/text", nvgRGB(230, 240, 235));
    dark.addColor("brls/text_disabled", nvgRGB(110, 130, 120));
    dark.addColor("brls/accent", nvgRGB(72, 168, 140));
    dark.addColor("brls/list/listItem_value_color", nvgRGB(150, 190, 170));
    dark.addColor("brls/header/border", nvgRGBA(72, 168, 140, 90));
    dark.addColor("brls/sidebar/background", nvgRGB(16, 24, 28));
    dark.addColor("brls/sidebar/active_item", nvgRGB(72, 168, 140));
    dark.addColor("brls/applet_frame/separator", nvgRGBA(72, 168, 140, 70));

    auto& light = brls::Theme::getLightTheme();
    light.addColor("brls/accent", nvgRGB(32, 120, 96));
    light.addColor("brls/background", nvgRGB(236, 244, 240));

    brls::Application::getPlatform()->setThemeVariant(brls::ThemeVariant::DARK);

    brls::Application::registerXMLView("PokemonSlotView", PokemonSlotView::create);
    brls::Application::registerXMLView("BoxGridView", BoxGridView::create);
#endif
}

}  // namespace pkhub::ui
