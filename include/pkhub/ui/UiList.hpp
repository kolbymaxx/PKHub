#pragma once

#if defined(PKHUB_HAS_BOREALIS)

#include <borealis.hpp>
#include <borealis/views/cells/cell_detail.hpp>
#include <functional>
#include <string>
#include <utility>

namespace pkhub::ui {

/// Scrolling column list used instead of removed brls::List.
struct ScrollList {
    brls::ScrollingFrame* scroll = nullptr;
    brls::Box* content = nullptr;
};

inline ScrollList makeScrollList() {
    ScrollList out;
    out.scroll = new brls::ScrollingFrame();
    out.scroll->setGrow(1.0f);
    out.content = new brls::Box(brls::Axis::COLUMN);
    out.content->setAlignItems(brls::AlignItems::STRETCH);
    auto style = brls::getStyle();
    out.content->setPadding(style["brls/sidebar/padding_top"],
                            style["brls/sidebar/padding_right"],
                            style["brls/sidebar/padding_bottom"],
                            style["brls/sidebar/padding_left"]);
    out.scroll->setContentView(out.content);
    return out;
}

inline brls::DetailCell* makeDetailCell(const std::string& title,
                                        const std::string& detail = {}) {
    auto* cell = new brls::DetailCell();
    cell->setText(title);
    if (!detail.empty()) {
        cell->setDetailText(detail);
    }
    return cell;
}

inline brls::Header* makeSectionHeader(const std::string& title,
                                       const std::string& subtitle = {}) {
    auto* h = new brls::Header();
    h->setTitle(title);
    if (!subtitle.empty()) {
        h->setSubtitle(subtitle);
    }
    return h;
}

/// Tip / prose block — never put long copy in DetailCell title+detail.
inline void addTipBlock(brls::Box* content,
                        const std::string& title,
                        const std::string& body) {
    content->addView(makeSectionHeader(title, body));
}

inline void addBodyLabel(brls::Box* content, const std::string& text) {
    auto* wrap = new brls::Box(brls::Axis::COLUMN);
    wrap->setPadding(2, 8, 8, 8);
    auto* label = new brls::Label();
    label->setFontSize(15);
    label->setTextColor(nvgRGBA(170, 200, 185, 230));
    label->setText(text);
    wrap->addView(label);
    content->addView(wrap);
}

inline void addClickableDetail(brls::Box* content,
                               const std::string& title,
                               const std::string& detail,
                               std::function<bool(brls::View*)> onClick) {
    auto* cell = makeDetailCell(title, detail);
    if (onClick) {
        cell->registerClickAction(std::move(onClick));
    }
    content->addView(cell);
}

/// AppletFrame::setContentView is protected — use the content ctor + setTitle.
inline brls::AppletFrame* makeAppletFrame(const std::string& title, brls::View* content) {
    auto* frame = new brls::AppletFrame(content);
    frame->setTitle(title);
    return frame;
}

}  // namespace pkhub::ui

#endif
