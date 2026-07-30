#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "pkhub/core/pokemon/Pokemon.hpp"

namespace pkhub::hub_codec {

/// Serialize a Pokémon into the Phase-1 unified hub payload blob.
std::vector<uint8_t> encodePokemon(const Pokemon& mon);

/// Decode a unified hub payload into `out`. Returns false on truncated/invalid data.
bool decodePokemon(const uint8_t* data, std::size_t len, Pokemon& out);

inline bool decodePokemon(const std::vector<uint8_t>& data, Pokemon& out) {
    return decodePokemon(data.data(), data.size(), out);
}

}  // namespace pkhub::hub_codec
