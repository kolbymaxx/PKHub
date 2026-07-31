#include "pkhub/backends/switch/SwitchSaveParser.hpp"

#include <cstring>

namespace pkhub {

std::size_t switchBoxCountFor(GameId game) {
    switch (game) {
        case GameId::Sword:
        case GameId::Shield:
            return 32;
        case GameId::BrilliantDiamond:
        case GameId::ShiningPearl:
            return 40;
        case GameId::LegendsArceus:
            // LA uses pastures; expose a fixed grid for Phase 1 UI.
            return 32;
        case GameId::Scarlet:
        case GameId::Violet:
            return 32;
        case GameId::LegendsZA:
            return 32;
        default:
            return 32;
    }
}

SwitchParseResult parseSwitchSave(GameId game, const std::vector<uint8_t>& data) {
    SwitchParseResult result;
    const std::size_t boxesN = switchBoxCountFor(game);
    result.boxes.assign(boxesN, Box{kDefaultBoxSlots});
    for (std::size_t i = 0; i < result.boxes.size(); ++i) {
        result.boxes[i].setName("Box " + std::to_string(i + 1));
    }
    result.party = Party{};

    if (data.empty()) {
        result.message = "Empty save buffer";
        return result;
    }

    // Phase 1: crypto/structure parse for SV/SwSh/etc. is not implemented yet.
    // We still open empty editable boxes so Hub transfer UI and mount plumbing can be tested.
    result.ok = true;
    result.parseImplemented = false;
    result.message =
        "Mounted " + std::to_string(data.size()) +
        " bytes for " + std::string(gameDisplayName(game)) +
        ". Pokémon decrypt/parse not implemented yet — showing empty boxes. "
        "Do not expect in-game Pokémon until the parser lands.";
    return result;
}

bool serializeSwitchSave(GameId game,
                         const std::vector<uint8_t>& original,
                         const Party& party,
                         const std::vector<Box>& boxes,
                         std::vector<uint8_t>& out,
                         std::string* err) {
    (void)game;
    (void)party;
    (void)boxes;

    if (original.empty()) {
        if (err) {
            *err = "No original save buffer to write";
        }
        return false;
    }

    // Refuse to invent encrypted SV/SwSh bytes. Preserve original until parser exists.
    // If any box/party slot is occupied, block write to avoid silent data loss illusions.
    bool anyOccupied = party.occupiedCount() > 0;
    for (const auto& b : boxes) {
        if (b.occupiedCount() > 0) {
            anyOccupied = true;
            break;
        }
    }
    if (anyOccupied) {
        if (err) {
            *err =
                "Cannot write Pokémon into Switch saves until the game parser/serializer "
                "is implemented. Hub Storage can still hold Pokémon.";
        }
        return false;
    }

    out = original;
    return true;
}

}  // namespace pkhub
