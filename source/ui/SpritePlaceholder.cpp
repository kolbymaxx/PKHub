#include "pkhub/ui/SpritePlaceholder.hpp"

#include <algorithm>
#include <string>

namespace pkhub {

SpritePlaceholder emptySlotPlaceholder() {
    return {40, 42, 48, 180, {}};
}

SpritePlaceholder placeholderFor(const Pokemon& mon) {
    if (mon.empty()) {
        return emptySlotPlaceholder();
    }

    // Deterministic pastel-ish color from species id (no asset deps).
    const uint32_t h = static_cast<uint32_t>(mon.species) * 2654435761u;
    SpritePlaceholder p;
    p.r = static_cast<uint8_t>(64 + (h & 0x7F));
    p.g = static_cast<uint8_t>(64 + ((h >> 8) & 0x7F));
    p.b = static_cast<uint8_t>(64 + ((h >> 16) & 0x7F));
    p.a = 255;
    p.label = std::to_string(mon.species);
    if (mon.isShiny) {
        // Slight gold bias for shiny placeholders.
        p.r = static_cast<uint8_t>(std::min(255, static_cast<int>(p.r) + 40));
        p.g = static_cast<uint8_t>(std::min(255, static_cast<int>(p.g) + 30));
    }
    return p;
}

}  // namespace pkhub
