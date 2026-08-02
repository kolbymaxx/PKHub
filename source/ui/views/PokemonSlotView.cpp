#include "pkhub/ui/views/PokemonSlotView.hpp"

#include "pkhub/ui/SpritePlaceholder.hpp"
#include "pkhub/ui/SpriteService.hpp"

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

PokemonSlotView::PokemonSlotView() {
    setAxis(brls::Axis::COLUMN);
    setJustifyContent(brls::JustifyContent::CENTER);
    setAlignItems(brls::AlignItems::CENTER);
    setWidth(86);
    setHeight(104);
    setCornerRadius(12);
    setBackgroundColor(nvgRGBA(16, 24, 30, 220));
    setBorderThickness(1.2f);
    setBorderColor(nvgRGBA(90, 140, 120, 80));
    setMarginRight(6);
    setMarginBottom(4);

    frame_ = new brls::Box(brls::Axis::COLUMN);
    frame_->setJustifyContent(brls::JustifyContent::CENTER);
    frame_->setAlignItems(brls::AlignItems::CENTER);
    frame_->setWidth(68);
    frame_->setHeight(68);
    frame_->setCornerRadius(10);
    frame_->setBackgroundColor(nvgRGB(24, 34, 42));
    addView(frame_);

    image_ = new brls::Image();
    image_->setWidth(58);
    image_->setHeight(58);
    image_->setScalingType(brls::ImageScalingType::FIT);
    image_->setVisibility(brls::Visibility::GONE);
    frame_->addView(image_);

    fallback_ = new brls::Rectangle(nvgRGB(42, 52, 60));
    fallback_->setWidth(48);
    fallback_->setHeight(48);
    fallback_->setCornerRadius(8);
    frame_->addView(fallback_);

    shinyBadge_ = new brls::Label();
    shinyBadge_->setFontSize(13);
    shinyBadge_->setText("✦");
    shinyBadge_->setTextColor(nvgRGB(245, 210, 90));
    shinyBadge_->setVisibility(brls::Visibility::GONE);
    addView(shinyBadge_);

    label_ = new brls::Label();
    label_->setFontSize(12);
    label_->setTextColor(nvgRGB(200, 220, 210));
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
            fallback_->setColor(nvgRGBA(36, 46, 54, 160));
        }
        if (label_) {
            label_->setText("·");
            label_->setTextColor(nvgRGBA(100, 130, 118, 180));
        }
        if (shinyBadge_) {
            shinyBadge_->setVisibility(brls::Visibility::GONE);
        }
        setBackgroundColor(nvgRGBA(14, 20, 26, 160));
        setBorderColor(nvgRGBA(70, 95, 88, 55));
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
        label_->setTextColor(nvgRGB(210, 228, 218));
        label_->setText(SpriteService::displayLabel(*mon));
    }
    if (shinyBadge_) {
        shinyBadge_->setVisibility(mon->isShiny ? brls::Visibility::VISIBLE
                                                : brls::Visibility::GONE);
    }
    setBackgroundColor(nvgRGBA(18, 28, 34, 235));
    setBorderColor(mon->isShiny ? nvgRGBA(235, 195, 70, 190) : nvgRGBA(90, 170, 140, 120));
    setBorderThickness(mon->isShiny ? 2.0f : 1.2f);
}

void PokemonSlotView::setSelected(bool selected) {
    setBackgroundColor(selected ? nvgRGBA(36, 78, 66, 245) : nvgRGBA(16, 24, 30, 220));
    setBorderThickness(selected ? 2.4f : 1.2f);
}

#endif

}  // namespace pkhub::ui
