#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/core/save/ISaveBackend.hpp"

namespace pkhub {

struct SwitchParseResult {
    bool ok = false;
    bool parseImplemented = false;
    std::string message;
    std::vector<Box> boxes;
    Party party;
};

/// Box count for official titles (PC storage).
std::size_t switchBoxCountFor(GameId game);

/**
 * Parse mounted Switch save bytes into boxes/party.
 * SwSh / SV: SwishCrypto + PK8/PK9 party slots.
 * BDSP / LA: mount works; full entity parse follows (LA uses PA8).
 */
SwitchParseResult parseSwitchSave(GameId game, const std::vector<uint8_t>& data);

/**
 * Serialize boxes/party back to save bytes (SwishCrypto re-encrypt for SwSh/SV).
 */
bool serializeSwitchSave(GameId game,
                         const std::vector<uint8_t>& original,
                         const Party& party,
                         const std::vector<Box>& boxes,
                         std::vector<uint8_t>& out,
                         std::string* err);

}  // namespace pkhub
