#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/core/pokemon/Pokemon.hpp"
#include "pkhub/core/save/ISaveBackend.hpp"

namespace pkhub::gba {

constexpr std::size_t kGbaSaveSize = 0x20000;
constexpr std::size_t kSectionSize = 0x1000;
constexpr std::size_t kSectionsPerSlot = 14;
constexpr std::size_t kSlotSize = kSectionSize * kSectionsPerSlot;  // 0xE000
constexpr std::size_t kBoxMonSize = 80;
constexpr std::size_t kPartyMonSize = 100;
constexpr std::size_t kBoxCount = 14;
constexpr std::size_t kSlotsPerBox = 30;
constexpr uint32_t kSectionSignature = 0x08012025;

struct GbaParseResult {
    GameId game = GameId::Unknown;
    Party party;
    std::vector<Box> boxes;
    std::string message;
    bool ok = false;
};

/// Lightweight Gen 3 probe: section magic `0x08012025` + trainer/team game detect.
struct GbaProbeResult {
    bool looksLikeGba = false;
    GameId game = GameId::Unknown;
    int validSections = 0;
    uint32_t saveIndex = 0;
};

GbaProbeResult probeSave(const std::vector<uint8_t>& data);

/// Parse a Gen 3 GBA save (.sav / .srm). Uses the first 0x20000 bytes when larger (RTC footer).
GbaParseResult parseSave(const std::vector<uint8_t>& data);

/// Decode an 80-byte box Pokémon into the unified model. Returns false if empty/invalid.
bool decodeBoxMon(const uint8_t* src80, Pokemon& out);

/// Best-effort encode of species/shiny/level/iv/ev/moves/pid/ot into an 80-byte box mon.
bool encodeBoxMon(const Pokemon& in, uint8_t* dst80);

/// Decode party mon (100 bytes); uses the trailing battle stats for level when present.
bool decodePartyMon(const uint8_t* src100, Pokemon& out);

/// Encode party mon (100 bytes): box structure + minimal battle-stats trailer.
bool encodePartyMon(const Pokemon& in, uint8_t* dst100);

/**
 * Write party + PC boxes back into a Gen 3 save buffer in-place.
 * Updates the active slot (higher save index), section checksums, and bumps saveIndex.
 * Preserves bytes beyond 0x20000 (RTC footer). Returns false on failure.
 */
bool writeSave(std::vector<uint8_t>& data,
               GameId game,
               const Party& party,
               const std::vector<Box>& boxes,
               std::string* err = nullptr);

}  // namespace pkhub::gba
