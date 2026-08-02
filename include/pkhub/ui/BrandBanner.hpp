#pragma once

#if defined(PKHUB_HAS_BOREALIS)

#include <borealis.hpp>
#include <functional>
#include <string>

#include "pkhub/ui/ThemeTokens.hpp"

namespace pkhub::ui {

/// Compact brand strip for tab tops — product name first, one short line.
inline brls::Box* makeBrandBanner(const std::string& subtitle) {
    auto* wrap = new brls::Box(brls::Axis::COLUMN);
    wrap->setPadding(2, 2, 12, 2);

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
    brand->setFontSize(34);
    brand->setTextColor(theme::text());
    brand->setText("PKHub");
    textCol->addView(brand);

    auto* sub = new brls::Label();
    sub->setFontSize(15);
    sub->setTextColor(theme::textMuted());
    sub->setText(subtitle);
    textCol->addView(sub);

    row->addView(textCol);
    wrap->addView(row);

    auto* rule = new brls::Rectangle(theme::rule());
    rule->setHeight(2);
    rule->setMarginTop(12);
    wrap->addView(rule);
    return wrap;
}

/// HOME-style game row: title + short status (e.g. "Open PC").
inline brls::Box* makeGameOpenRow(const std::string& title,
                                  const std::string& status,
                                  std::function<bool(brls::View*)> onClick) {
    auto* row = new brls::Box(brls::Axis::ROW);
    row->setAlignItems(brls::AlignItems::CENTER);
    row->setPadding(14, 12, 14, 12);
    row->setMarginBottom(8);
    row->setCornerRadius(12);
    row->setBackgroundColor(theme::bgPanel());
    row->setBorderThickness(1.0f);
    row->setBorderColor(theme::accentDim());
    if (onClick) {
        row->registerClickAction(std::move(onClick));
    }

    auto* col = new brls::Box(brls::Axis::COLUMN);
    col->setGrow(1.f);

    auto* name = new brls::Label();
    name->setFontSize(20);
    name->setTextColor(theme::text());
    name->setText(title);
    col->addView(name);

    auto* st = new brls::Label();
    st->setFontSize(14);
    st->setTextColor(theme::textMuted());
    st->setText(status);
    col->addView(st);
    row->addView(col);

    auto* chevron = new brls::Label();
    chevron->setFontSize(22);
    chevron->setTextColor(theme::accent());
    chevron->setText("›");
    row->addView(chevron);
    return row;
}

}  // namespace pkhub::ui

#endif
