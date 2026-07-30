#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "pkhub/core/pokemon/GameId.hpp"

namespace pkhub {

enum class PokemonType : uint8_t {
    Normal = 0,
    Fighting,
    Flying,
    Poison,
    Ground,
    Rock,
    Bug,
    Ghost,
    Steel,
    Fire,
    Water,
    Grass,
    Electric,
    Psychic,
    Ice,
    Dragon,
    Dark,
    Fairy,
    Stellar,  // Gen 9 Tera
    None = 255,
};

struct Stats {
    uint8_t hp{};
    uint8_t atk{};
    uint8_t def{};
    uint8_t spa{};
    uint8_t spd{};
    uint8_t spe{};
};

/**
 * Unified, generation-aware Pokémon model.
 * Backends map native structures ↔ Pokemon.
 * Fields that do not exist for a generation are ignored on serialize.
 */
class Pokemon {
public:
    bool empty() const { return species == 0; }
    void clear();

    Generation originGeneration() const;

    // Identity
    uint16_t species = 0;
    uint8_t form = 0;
    bool isEgg = false;
    bool isShiny = false;
    bool isAlpha = false;  // LA / relevant titles

    // Identity strings
    std::string nickname;
    std::string otName;
    uint32_t otId = 0;
    uint32_t otSecretId = 0;
    uint32_t pid = 0;

    // Growth
    uint8_t level = 1;
    uint32_t exp = 0;
    uint8_t friendship = 0;
    int8_t nature = -1;   // -1 = unset / follow PID where applicable
    int8_t abilitySlot = -1;
    uint16_t abilityId = 0;
    uint8_t gender = 0;   // game-specific encoding documented in impl
    uint8_t ball = 0;

    // Combat
    Stats ivs{};
    Stats evs{};
    std::array<uint16_t, 4> moves{};
    std::array<uint8_t, 4> pp{};
    std::array<uint8_t, 4> ppUps{};

    // Held
    uint16_t heldItem = 0;

    // Gen 8+
    uint8_t gmaxFactor = 0;

    // Gen 9
    PokemonType teraType = PokemonType::None;

    // Met data (simplified; expanded per gen in Phase 2)
    uint16_t metLocation = 0;
    uint8_t metLevel = 0;
    uint32_t metDate = 0;  // packed YYYYMMDD or game-native; documented in impl
    GameId originGame = GameId::Unknown;

    // Opaque native blob for lossless round-trip when unified fields are incomplete
    std::vector<uint8_t> nativeBlob;
    Generation nativeGeneration = Generation::Unknown;
};

}  // namespace pkhub
