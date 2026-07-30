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

/// Parse a Gen 3 GBA save (.sav / .srm). Uses the first 0x20000 bytes when larger (RTC footer).
GbaParseResult parseSave(const std::vector<uint8_t>& data);

/// Decode an 80-byte box Pokémon into the unified model. Returns false if empty/invalid.
bool decodeBoxMon(const uint8_t* src80, Pokemon& out);

/// Best-effort encode of species/shiny/level/iv/ev/moves/pid/ot into an 80-byte box mon.
bool encodeBoxMon(const Pokemon& in, uint8_t* dst80);

/// Decode party mon (100 bytes); uses the trailing battle stats for level when present.
bool decodePartyMon(const uint8_t* src100, Pokemon& out);

}  // namespace pkhub::gba
