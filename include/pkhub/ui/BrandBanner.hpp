#pragma once

#if defined(PKHUB_HAS_BOREALIS)

#include <borealis.hpp>
#include <string>

namespace pkhub::ui {

/// Compact brand strip for tab tops — product name first, one short line.
inline brls::Box* makeBrandBanner(const std::string& subtitle) {
    auto* wrap = new brls::Box(brls::Axis::COLUMN);
    wrap->setPadding(8, 4, 18, 4);

    auto* row = new brls::Box(brls::Axis::ROW);
    row->setAlignItems(brls::AlignItems::CENTER);

    auto* icon = new brls::Image();
    icon->setWidth(48);
    icon->setHeight(48);
    icon->setCornerRadius(12);
    icon->setScalingType(brls::ImageScalingType::FIT);
    icon->setImageFromRes("img/icons/pkhub.png");
    row->addView(icon);

    auto* textCol = new brls::Box(brls::Axis::COLUMN);
    textCol->setPaddingLeft(14);

    auto* brand = new brls::Label();
    brand->setFontSize(28);
    brand->setTextColor(nvgRGB(235, 248, 242));
    brand->setText("PKHub");
    textCol->addView(brand);

    auto* sub = new brls::Label();
    sub->setFontSize(14);
    sub->setTextColor(nvgRGBA(130, 185, 165, 230));
    sub->setText(subtitle);
    textCol->addView(sub);

    row->addView(textCol);
    wrap->addView(row);

    auto* rule = new brls::Rectangle(nvgRGBA(72, 168, 140, 70));
    rule->setHeight(2);
    rule->setMarginTop(14);
    wrap->addView(rule);
    return wrap;
}

}  // namespace pkhub::ui

#endif
