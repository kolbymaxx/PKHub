#pragma once

#include <cstdint>
#include <string>

#include "pkhub/core/pokemon/Pokemon.hpp"

namespace pkhub {

/// English National Dex display name for id 1..1025 (empty/unknown → blank).
const char* speciesEnglishName(uint16_t nationalId);

/// Best UI label: nickname if set, else English species name, else "#nnn".
std::string pokemonDisplayName(const Pokemon& mon);

}  // namespace pkhub
