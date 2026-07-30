#include "pkhub/ui/views/PokemonSlotView.hpp"

#include "pkhub/ui/SpritePlaceholder.hpp"

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

PokemonSlotView::PokemonSlotView() {
    setAxis(brls::Axis::COLUMN);
    setJustifyContent(brls::JustifyContent::CENTER);
    setAlignItems(brls::AlignItems::CENTER);
    setWidth(72);
    setHeight(84);
    setCornerRadius(6);
    setBackgroundColor(nvgRGB(36, 38, 44));

    swatch_ = new brls::Rectangle(nvgRGB(60, 62, 70));
    swatch_->setWidth(52);
    swatch_->setHeight(52);
    swatch_->setCornerRadius(4);
    addView(swatch_);

    label_ = new brls::Label();
    label_->setFontSize(14);
    label_->setText("");
    addView(label_);
}

brls::View* PokemonSlotView::create() {
    return new PokemonSlotView();
}

void PokemonSlotView::setPokemon(const Pokemon* mon) {
    const SpritePlaceholder ph = mon ? placeholderFor(*mon) : emptySlotPlaceholder();
    if (swatch_) {
        swatch_->setColor(nvgRGBA(ph.r, ph.g, ph.b, ph.a));
    }
    if (label_) {
        label_->setText(ph.label.empty() ? "-" : ph.label);
    }
}

void PokemonSlotView::setSelected(bool selected) {
    setBackgroundColor(selected ? nvgRGB(70, 90, 120) : nvgRGB(36, 38, 44));
}

#endif

}  // namespace pkhub::ui
