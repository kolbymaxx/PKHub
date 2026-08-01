#include "pkhub/ui/SpriteService.hpp"

#include <cstdio>

#include "pkhub/core/pokemon/SpeciesIds.hpp"

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
    char buf[32];
    std::snprintf(buf, sizeof(buf), "#%u", unsigned(nationalId(mon)));
    return buf;
}

}  // namespace pkhub
