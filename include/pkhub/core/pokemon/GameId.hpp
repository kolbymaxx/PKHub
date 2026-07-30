#pragma once

#include <cstdint>
#include <string>

namespace pkhub {

/// Logical game identity used across backends and UI.
enum class GameId : uint16_t {
    Unknown = 0,

    // Gen 3
    Ruby,
    Sapphire,
    Emerald,
    FireRed,
    LeafGreen,

    // Gen 4
    Diamond,
    Pearl,
    Platinum,
    HeartGold,
    SoulSilver,

    // Gen 5
    Black,
    White,
    Black2,
    White2,

    // Gen 6
    X,
    Y,
    OmegaRuby,
    AlphaSapphire,

    // Gen 7
    Sun,
    Moon,
    UltraSun,
    UltraMoon,

    // Gen 8 Switch
    Sword,
    Shield,
    BrilliantDiamond,
    ShiningPearl,
    LegendsArceus,

    // Gen 9 Switch
    Scarlet,
    Violet,
    LegendsZA,
};

enum class Generation : uint8_t {
    Unknown = 0,
    Gen3 = 3,
    Gen4 = 4,
    Gen5 = 5,
    Gen6 = 6,
    Gen7 = 7,
    Gen8 = 8,
    Gen9 = 9,
};

Generation generationFor(GameId id);
const char* gameDisplayName(GameId id);

/// Official Switch application IDs (0 if not a Switch title).
uint64_t switchTitleId(GameId id);

}  // namespace pkhub
