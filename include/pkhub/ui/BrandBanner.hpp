#pragma once

#if defined(PKHUB_HAS_BOREALIS)

#include <borealis.hpp>
#include <string>

namespace pkhub::ui {

/// Compact brand strip for tab tops — product name first, one short line.
inline brls::Box* makeBrandBanner(const std::string& subtitle) {
    auto* wrap = new brls::Box(brls::Axis::COLUMN);
    wrap->setPadding(4, 2, 14, 2);

    auto* row = new brls::Box(brls::Axis::ROW);
    row->setAlignItems(brls::AlignItems::CENTER);

    auto* icon = new brls::Image();
    icon->setWidth(56);
    icon->setHeight(56);
    icon->setCornerRadius(14);
    icon->setScalingType(brls::ImageScalingType::FIT);
    icon->setImageFromRes("img/icons/pkhub.png");
    row->addView(icon);

    auto* textCol = new brls::Box(brls::Axis::COLUMN);
    textCol->setPaddingLeft(16);

    auto* brand = new brls::Label();
    brand->setFontSize(32);
    brand->setTextColor(nvgRGB(235, 248, 242));
    brand->setText("PKHub");
    textCol->addView(brand);

    auto* sub = new brls::Label();
    sub->setFontSize(15);
    sub->setTextColor(nvgRGBA(120, 190, 165, 235));
    sub->setText(subtitle);
    textCol->addView(sub);

    row->addView(textCol);
    wrap->addView(row);

    auto* rule = new brls::Rectangle(nvgRGBA(78, 176, 148, 90));
    rule->setHeight(2);
    rule->setMarginTop(12);
    wrap->addView(rule);
    return wrap;
}

}  // namespace pkhub::ui

#endif
