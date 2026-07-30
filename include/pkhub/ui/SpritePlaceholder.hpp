#pragma once

#include <cstdint>
#include <string>

#include "pkhub/core/pokemon/Pokemon.hpp"

namespace pkhub {

/// Phase 1: simple colored placeholders instead of real sprites.
struct SpritePlaceholder {
    uint8_t r = 80;
    uint8_t g = 80;
    uint8_t b = 90;
    uint8_t a = 255;
    std::string label;  // e.g. species number or "?" 
};

SpritePlaceholder placeholderFor(const Pokemon& mon);
SpritePlaceholder emptySlotPlaceholder();

}  // namespace pkhub
