#pragma once

#include <cstdint>

#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/core/pokemon/Pokemon.hpp"

namespace pkhub {

/// SV ROM internal index → National Dex (Bulbapedia Gen IX index list + PokeAPI).
/// Identity for 1–916; compact remap for 917–1025.
uint16_t svInternalToNational(uint16_t internal);

/// Best-effort National Dex id for sprite/label display.
/// SwSh and earlier: `species` is already national. SV: remap internals.
uint16_t nationalDexId(const Pokemon& mon);

}  // namespace pkhub
