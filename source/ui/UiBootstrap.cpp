#include "pkhub/ui/UiBootstrap.hpp"

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
#include "pkhub/ui/views/BoxGridView.hpp"
#include "pkhub/ui/views/PokemonSlotView.hpp"
#endif

namespace pkhub::ui {

void registerCustomViews() {
#if defined(PKHUB_HAS_BOREALIS)
    brls::Application::registerXMLView("PokemonSlotView", PokemonSlotView::create);
    brls::Application::registerXMLView("BoxGridView", BoxGridView::create);
#endif
}

}  // namespace pkhub::ui
