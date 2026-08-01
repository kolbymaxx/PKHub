#pragma once

#include <cstdint>
#include <string>

#include "pkhub/core/pokemon/Pokemon.hpp"

namespace pkhub {

/// Resolve romfs/resource paths for Pokémon sprites (PokeAPI-style national IDs).
class SpriteService {
public:
    static SpriteService& instance();

    /// National dex id used for sprite filenames. Falls back to stored species.
    static uint16_t nationalId(const Pokemon& mon);

    /// Relative resource path, e.g. "img/pokemon/25.png" or shiny variant.
    static std::string spritePath(const Pokemon& mon);
    static std::string spritePath(uint16_t national, bool shiny);

    /// Short display label for UI (species number until name tables land).
    static std::string displayLabel(const Pokemon& mon);
};

}  // namespace pkhub
