#pragma once

namespace pkhub {

/// Known official Switch Pokémon title IDs (base games).
namespace title_ids {
constexpr uint64_t Sword            = 0x0100ABF008968000ULL;
constexpr uint64_t Shield           = 0x01008DB008C2C000ULL;
constexpr uint64_t BrilliantDiamond = 0x0100000011D90000ULL;
constexpr uint64_t ShiningPearl     = 0x010018E011D92000ULL;
constexpr uint64_t LegendsArceus    = 0x01001F5010DFA000ULL;
constexpr uint64_t Scarlet          = 0x0100A3D008C5C000ULL;
constexpr uint64_t Violet           = 0x01008F6008C5E000ULL;
// Legends Z-A — confirm against final retail title ID before shipping detection.
constexpr uint64_t LegendsZA        = 0x0000000000000000ULL;
}  // namespace title_ids

}  // namespace pkhub
