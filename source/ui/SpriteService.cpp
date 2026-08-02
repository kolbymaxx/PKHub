#include "pkhub/ui/SpriteService.hpp"

#include <cstdio>

#include "pkhub/core/pokemon/SpeciesIds.hpp"
#include "pkhub/core/pokemon/SpeciesNames.hpp"

namespace pkhub {

SpriteService& SpriteService::instance() {
    static SpriteService s;
    return s;
}

uint16_t SpriteService::nationalId(const Pokemon& mon) {
    return nationalDexId(mon);
}

std::string SpriteService::spritePath(uint16_t national, bool shiny) {
    if (national == 0) {
        return {};
    }
    char buf[64];
    if (shiny) {
        std::snprintf(buf, sizeof(buf), "img/pokemon/shiny/%u.png", unsigned(national));
    } else {
        std::snprintf(buf, sizeof(buf), "img/pokemon/%u.png", unsigned(national));
    }
    return buf;
}

std::string SpriteService::spritePath(const Pokemon& mon) {
    if (mon.empty()) {
        return {};
    }
    return spritePath(nationalId(mon), mon.isShiny);
}

std::string SpriteService::displayLabel(const Pokemon& mon) {
    if (mon.empty()) {
        return {};
    }
    std::string name = pokemonDisplayName(mon);
    // Box slots are narrow — keep labels short.
    if (name.size() > 10) {
        name.resize(9);
        name += "...";
    }
    return name;
}

}  // namespace pkhub
