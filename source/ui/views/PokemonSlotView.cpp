#include "pkhub/ui/views/PokemonSlotView.hpp"

#include "pkhub/ui/SpritePlaceholder.hpp"
#include "pkhub/ui/SpriteService.hpp"
#include "pkhub/ui/ThemeTokens.hpp"

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

PokemonSlotView::PokemonSlotView() {
    setAxis(brls::Axis::COLUMN);
    setJustifyContent(brls::JustifyContent::CENTER);
    setAlignItems(brls::AlignItems::CENTER);
    setWidth(88);
    setHeight(100);
    setCornerRadius(10);
    setBackgroundColor(theme::slotEmpty());
    setBorderThickness(1.2f);
    setBorderColor(nvgRGBA(55, 90, 80, 70));
    setMarginRight(5);
    setMarginBottom(5);

    frame_ = new brls::Box(brls::Axis::COLUMN);
    frame_->setJustifyContent(brls::JustifyContent::CENTER);
    frame_->setAlignItems(brls::AlignItems::CENTER);
    frame_->setWidth(70);
    frame_->setHeight(70);
    frame_->setCornerRadius(35);
    frame_->setBackgroundColor(nvgRGBA(20, 36, 40, 220));
    addView(frame_);

    image_ = new brls::Image();
    image_->setWidth(60);
    image_->setHeight(60);
    image_->setScalingType(brls::ImageScalingType::FIT);
    image_->setVisibility(brls::Visibility::GONE);
    frame_->addView(image_);

    fallback_ = new brls::Rectangle(nvgRGB(36, 48, 52));
    fallback_->setWidth(40);
    fallback_->setHeight(40);
    fallback_->setCornerRadius(20);
    frame_->addView(fallback_);

    shinyBadge_ = new brls::Label();
    shinyBadge_->setFontSize(12);
    shinyBadge_->setText("✦");
    shinyBadge_->setTextColor(theme::shiny());
    shinyBadge_->setVisibility(brls::Visibility::GONE);
    addView(shinyBadge_);

    label_ = new brls::Label();
    label_->setFontSize(11);
    label_->setTextColor(theme::textMuted());
    label_->setText("");
    addView(label_);
}

brls::View* PokemonSlotView::create() {
    return new PokemonSlotView();
}

void PokemonSlotView::setPokemon(const Pokemon* mon) {
    const bool empty = !mon || mon->empty();
    if (empty) {
        if (image_) {
            image_->setVisibility(brls::Visibility::GONE);
        }
        if (fallback_) {
            fallback_->setVisibility(brls::Visibility::VISIBLE);
            fallback_->setColor(nvgRGBA(28, 40, 44, 140));
        }
        if (label_) {
            label_->setText("");
            label_->setTextColor(theme::textFaint());
        }
        if (shinyBadge_) {
            shinyBadge_->setVisibility(brls::Visibility::GONE);
        }
        setBackgroundColor(theme::slotEmpty());
        setBorderColor(nvgRGBA(50, 75, 68, 50));
        setBorderThickness(1.0f);
        return;
    }

    const auto path = SpriteService::spritePath(*mon);
    bool loaded = false;
    if (image_ && !path.empty()) {
        image_->setImageFromRes(path);
        image_->setVisibility(brls::Visibility::VISIBLE);
        if (fallback_) {
            fallback_->setVisibility(brls::Visibility::GONE);
        }
        loaded = true;
    }
    if (!loaded) {
        const SpritePlaceholder ph = placeholderFor(*mon);
        if (image_) {
            image_->setVisibility(brls::Visibility::GONE);
        }
        if (fallback_) {
            fallback_->setVisibility(brls::Visibility::VISIBLE);
            fallback_->setColor(nvgRGBA(ph.r, ph.g, ph.b, ph.a));
        }
    }

    if (label_) {
        label_->setTextColor(theme::text());
        label_->setText(SpriteService::displayLabel(*mon));
    }
    if (shinyBadge_) {
        shinyBadge_->setVisibility(mon->isShiny ? brls::Visibility::VISIBLE
                                                : brls::Visibility::GONE);
    }
    setBackgroundColor(theme::slotFill());
    setBorderColor(mon->isShiny ? nvgRGBA(235, 195, 70, 200) : theme::slotBorder());
    setBorderThickness(mon->isShiny ? 2.0f : 1.2f);
}

void PokemonSlotView::setSelected(bool selected) {
    setBackgroundColor(selected ? nvgRGBA(36, 78, 66, 245) : theme::slotFill());
    setBorderThickness(selected ? 2.4f : 1.2f);
    setBorderColor(selected ? theme::accent() : theme::slotBorder());
}

#endif

}  // namespace pkhub::ui
