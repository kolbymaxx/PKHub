#pragma once

#include <cstddef>
#include <functional>

#include "pkhub/core/save/ISaveBackend.hpp"

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
#endif

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

class BoxGridView : public brls::Box {
public:
    BoxGridView();
    static brls::View* create();

    void bindProvider(IBoxProvider* provider, std::size_t boxIndex);
    void setBoxIndex(std::size_t boxIndex);
    std::size_t currentBoxIndex() const { return boxIndex_; }
    void refresh();

    std::function<void(std::size_t slotIndex)> onSlotActivated;

private:
    IBoxProvider* provider_ = nullptr;
    std::size_t boxIndex_ = 0;
    brls::Label* title_ = nullptr;
    brls::Box* grid_ = nullptr;
};

#else

class BoxGridView {
public:
    void bindProvider(IBoxProvider*, std::size_t) {}
    void setBoxIndex(std::size_t) {}
    std::size_t currentBoxIndex() const { return 0; }
    void refresh() {}
    std::function<void(std::size_t)> onSlotActivated;
};

#endif

}  // namespace pkhub::ui
