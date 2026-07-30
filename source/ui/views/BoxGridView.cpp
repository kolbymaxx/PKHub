#include "pkhub/ui/views/BoxGridView.hpp"

#include "pkhub/ui/views/PokemonSlotView.hpp"

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

BoxGridView::BoxGridView() {
    setAxis(brls::Axis::COLUMN);
    setPadding(20);
    setGrow(1.f);

    title_ = new brls::Label();
    title_->setFontSize(22);
    title_->setText("Box");
    addView(title_);

    grid_ = new brls::Box(brls::Axis::COLUMN);
    grid_->setGrow(1.f);
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

    const Box& box = static_cast<const IBoxProvider*>(provider_)->box(boxIndex_);
    if (title_) {
        title_->setText(box.name().empty() ? ("Box " + std::to_string(boxIndex_ + 1)) : box.name());
    }

    constexpr int kCols = 6;
    brls::Box* row = nullptr;
    for (std::size_t i = 0; i < box.size(); ++i) {
        if (i % kCols == 0) {
            row = new brls::Box(brls::Axis::ROW);
            row->setPaddingTop(8);
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
