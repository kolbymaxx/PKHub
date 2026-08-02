#pragma once

#include "pkhub/core/pokemon/Pokemon.hpp"

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
#endif

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

class PokemonSlotView : public brls::Box {
public:
    PokemonSlotView();
    static brls::View* create();

    void setPokemon(const Pokemon* mon);
    void setSelected(bool selected);

private:
    brls::Box* frame_ = nullptr;
    brls::Image* image_ = nullptr;
    brls::Rectangle* fallback_ = nullptr;
    brls::Label* label_ = nullptr;
    brls::Label* shinyBadge_ = nullptr;
};

#else

class PokemonSlotView {
public:
    void setPokemon(const Pokemon*) {}
    void setSelected(bool) {}
};

#endif

}  // namespace pkhub::ui
