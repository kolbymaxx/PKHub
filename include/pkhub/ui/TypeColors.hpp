#pragma once

#include "pkhub/core/pokemon/Pokemon.hpp"

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
#endif

namespace pkhub {

inline const char* pokemonTypeEnglishName(PokemonType t) {
    switch (t) {
        case PokemonType::Normal: return "Normal";
        case PokemonType::Fighting: return "Fighting";
        case PokemonType::Flying: return "Flying";
        case PokemonType::Poison: return "Poison";
        case PokemonType::Ground: return "Ground";
        case PokemonType::Rock: return "Rock";
        case PokemonType::Bug: return "Bug";
        case PokemonType::Ghost: return "Ghost";
        case PokemonType::Steel: return "Steel";
        case PokemonType::Fire: return "Fire";
        case PokemonType::Water: return "Water";
        case PokemonType::Grass: return "Grass";
        case PokemonType::Electric: return "Electric";
        case PokemonType::Psychic: return "Psychic";
        case PokemonType::Ice: return "Ice";
        case PokemonType::Dragon: return "Dragon";
        case PokemonType::Dark: return "Dark";
        case PokemonType::Fairy: return "Fairy";
        case PokemonType::Stellar: return "Stellar";
        default: return "—";
    }
}

#if defined(PKHUB_HAS_BOREALIS)
inline NVGcolor pokemonTypeColor(PokemonType t) {
    switch (t) {
        case PokemonType::Normal: return nvgRGB(168, 168, 120);
        case PokemonType::Fighting: return nvgRGB(192, 48, 40);
        case PokemonType::Flying: return nvgRGB(168, 144, 240);
        case PokemonType::Poison: return nvgRGB(160, 64, 160);
        case PokemonType::Ground: return nvgRGB(224, 192, 104);
        case PokemonType::Rock: return nvgRGB(184, 160, 56);
        case PokemonType::Bug: return nvgRGB(168, 184, 32);
        case PokemonType::Ghost: return nvgRGB(112, 88, 152);
        case PokemonType::Steel: return nvgRGB(184, 184, 208);
        case PokemonType::Fire: return nvgRGB(240, 128, 48);
        case PokemonType::Water: return nvgRGB(104, 144, 240);
        case PokemonType::Grass: return nvgRGB(120, 200, 80);
        case PokemonType::Electric: return nvgRGB(248, 208, 48);
        case PokemonType::Psychic: return nvgRGB(248, 88, 136);
        case PokemonType::Ice: return nvgRGB(152, 216, 216);
        case PokemonType::Dragon: return nvgRGB(112, 56, 248);
        case PokemonType::Dark: return nvgRGB(112, 88, 72);
        case PokemonType::Fairy: return nvgRGB(238, 153, 172);
        case PokemonType::Stellar: return nvgRGB(64, 180, 200);
        default: return nvgRGB(90, 110, 100);
    }
}
#endif

}  // namespace pkhub
