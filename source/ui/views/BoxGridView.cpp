#include "pkhub/ui/views/BoxGridView.hpp"

#include "pkhub/ui/ThemeTokens.hpp"
#include "pkhub/ui/views/PokemonSlotView.hpp"

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

BoxGridView::BoxGridView() {
    setAxis(brls::Axis::COLUMN);
    setPadding(8, 6, 6, 6);
    setGrow(1.f);
    setCornerRadius(16);
    setBackgroundColor(theme::bgWallpaper());
    setBorderThickness(1.0f);
    setBorderColor(theme::accentDim());

    auto* header = new brls::Box(brls::Axis::COLUMN);
    header->setPadding(4, 6, 2, 6);

    title_ = new brls::Label();
    title_->setFontSize(22);
    title_->setTextColor(theme::text());
    title_->setText("Box");
    header->addView(title_);

    auto* subtitle = new brls::Label();
    subtitle->setFontSize(13);
    subtitle->setTextColor(theme::textMuted());
    subtitle->setText("Organize Boxes · tap a Pokémon");
    header->addView(subtitle);
    addView(header);

    auto* divider = new brls::Rectangle(theme::rule());
    divider->setHeight(1);
    divider->setMarginTop(8);
    divider->setMarginBottom(4);
    addView(divider);

    grid_ = new brls::Box(brls::Axis::COLUMN);
    grid_->setGrow(1.f);
    grid_->setPaddingTop(6);
    grid_->setPaddingLeft(2);
    addView(grid_);
}

brls::View* BoxGridView::create() {
    return new BoxGridView();
}

void BoxGridView::bindProvider(IBoxProvider* provider, std::size_t boxIndex) {
    provider_ = provider;
    boxIndex_ = boxIndex;
    refresh();
}

void BoxGridView::setBoxIndex(std::size_t boxIndex) {
    boxIndex_ = boxIndex;
    refresh();
}

void BoxGridView::refresh() {
    if (!grid_) {
        return;
    }
    grid_->clearViews();
    if (!provider_ || boxIndex_ >= provider_->boxCount()) {
        if (title_) {
            title_->setText("No box");
        }
        return;
    }

    const pkhub::Box& box = provider_->box(boxIndex_);
    if (title_) {
        const std::string name =
            box.name().empty() ? ("Box " + std::to_string(boxIndex_ + 1)) : box.name();
        title_->setText(name + "  ·  " + std::to_string(box.occupiedCount()) + " Pokémon");
    }

    constexpr int kCols = 6;
    brls::Box* row = nullptr;
    for (std::size_t i = 0; i < box.size(); ++i) {
        if (i % kCols == 0) {
            row = new brls::Box(brls::Axis::ROW);
            row->setPaddingTop(2);
            row->setJustifyContent(brls::JustifyContent::FLEX_START);
            grid_->addView(row);
        }
        auto* slot = new PokemonSlotView();
        slot->setPokemon(&box.slot(i));
        const std::size_t slotIndex = i;
        slot->registerClickAction([this, slotIndex](brls::View*) {
            if (onSlotActivated) {
                onSlotActivated(slotIndex);
            }
            return true;
        });
        row->addView(slot);
    }
}

#endif

}  // namespace pkhub::ui
