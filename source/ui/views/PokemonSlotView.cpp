#include "pkhub/ui/views/PokemonSlotView.hpp"

#include "pkhub/ui/SpritePlaceholder.hpp"
#include "pkhub/ui/SpriteService.hpp"

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

PokemonSlotView::PokemonSlotView() {
    setAxis(brls::Axis::COLUMN);
    setJustifyContent(brls::JustifyContent::CENTER);
    setAlignItems(brls::AlignItems::CENTER);
    setWidth(78);
    setHeight(96);
    setCornerRadius(10);
    setBackgroundColor(nvgRGBA(18, 24, 32, 210));
    setBorderThickness(1.0f);
    setBorderColor(nvgRGBA(120, 160, 140, 70));

    frame_ = new brls::Box(brls::Axis::COLUMN);
    frame_->setJustifyContent(brls::JustifyContent::CENTER);
    frame_->setAlignItems(brls::AlignItems::CENTER);
    frame_->setWidth(64);
    frame_->setHeight(64);
    frame_->setCornerRadius(8);
    frame_->setBackgroundColor(nvgRGBA(28, 38, 48, 255));
    addView(frame_);

    image_ = new brls::Image();
    image_->setWidth(56);
    image_->setHeight(56);
    image_->setScalingType(brls::ImageScalingType::FIT);
    image_->setVisibility(brls::Visibility::GONE);
    frame_->addView(image_);

    fallback_ = new brls::Rectangle(nvgRGB(48, 58, 68));
    fallback_->setWidth(52);
    fallback_->setHeight(52);
    fallback_->setCornerRadius(6);
    frame_->addView(fallback_);

    shinyBadge_ = new brls::Label();
    shinyBadge_->setFontSize(12);
    shinyBadge_->setText("✦");
    shinyBadge_->setTextColor(nvgRGB(240, 200, 80));
    shinyBadge_->setVisibility(brls::Visibility::GONE);
    addView(shinyBadge_);

    label_ = new brls::Label();
    label_->setFontSize(13);
    label_->setTextColor(nvgRGB(210, 220, 215));
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
            fallback_->setColor(nvgRGBA(40, 48, 56, 180));
        }
        if (label_) {
            label_->setText("");
        }
        if (shinyBadge_) {
            shinyBadge_->setVisibility(brls::Visibility::GONE);
        }
        setBorderColor(nvgRGBA(80, 100, 95, 40));
        return;
    }

    const auto path = SpriteService::spritePath(*mon);
    bool loaded = false;
    if (image_ && !path.empty()) {
        image_->setImageFromRes(path);
        // NanoVG returns 0 texture on failure; borealis keeps prior texture — still show image view.
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
        label_->setText(SpriteService::displayLabel(*mon));
    }
    if (shinyBadge_) {
        shinyBadge_->setVisibility(mon->isShiny ? brls::Visibility::VISIBLE
                                                : brls::Visibility::GONE);
    }
    setBorderColor(mon->isShiny ? nvgRGBA(230, 190, 70, 160) : nvgRGBA(100, 170, 140, 90));
}

void PokemonSlotView::setSelected(bool selected) {
    setBackgroundColor(selected ? nvgRGBA(36, 70, 62, 240) : nvgRGBA(18, 24, 32, 210));
    setBorderThickness(selected ? 2.0f : 1.0f);
}

#endif

}  // namespace pkhub::ui
