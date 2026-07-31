#include "pkhub/backends/switch/SwitchSaveParser.hpp"

#include "pkhub/backends/switch/Pk8Codec.hpp"
#include "pkhub/backends/switch/PokeCrypto8.hpp"
#include "pkhub/backends/switch/SwishCrypto.hpp"

#include <algorithm>
#include <cstring>

namespace pkhub {
namespace {

// SCBlock keys (FNV-1a of official block names) — public format constants.
constexpr uint32_t kBoxSwShSv = 0x0d66012c;
constexpr uint32_t kPartySv = 0x3AA1A9AD;
constexpr uint32_t kPartySwSh = 0x2985fe5d;
constexpr uint32_t kBoxLayout = 0x19722c89;

bool usesSwishPk8(GameId game) {
    switch (game) {
        case GameId::Sword:
        case GameId::Shield:
        case GameId::Scarlet:
        case GameId::Violet:
            return true;
        default:
            return false;
    }
}

uint32_t partyKeyFor(GameId game) {
    switch (game) {
        case GameId::Scarlet:
        case GameId::Violet:
            return kPartySv;
        default:
            return kPartySwSh;
    }
}

Generation genFor(GameId game) {
    return generationFor(game);
}

void applyBoxNames(const ScBlock* layout, std::vector<Box>& boxes) {
    if (!layout || layout->data.empty()) {
        return;
    }
    // BoxLayout: UTF-16LE names, 13 chars (26 bytes) each typically.
    constexpr std::size_t kNameBytes = 26;
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        const std::size_t off = i * kNameBytes;
        if (off + 2 > layout->data.size()) {
            break;
        }
        std::string name;
        for (std::size_t c = 0; c + 1 < kNameBytes && off + c + 1 < layout->data.size(); c += 2) {
            const uint16_t ch = uint16_t(layout->data[off + c] | (layout->data[off + c + 1] << 8));
            if (ch == 0) {
                break;
            }
            if (ch < 128) {
                name.push_back(char(ch));
            }
        }
        if (!name.empty()) {
            boxes[i].setName(std::move(name));
        }
    }
}

SwitchParseResult parseSwish(GameId game, const std::vector<uint8_t>& data) {
    SwitchParseResult result;
    const std::size_t boxesN = switchBoxCountFor(game);
    result.boxes.assign(boxesN, Box{kDefaultBoxSlots});
    for (std::size_t i = 0; i < result.boxes.size(); ++i) {
        result.boxes[i].setName("Box " + std::to_string(i + 1));
    }

    std::vector<ScBlock> blocks;
    std::string err;
    if (!SwishCrypto::decrypt(data, blocks, &err)) {
        result.message = err;
        return result;
    }

    auto* boxBlock = SwishCrypto::findBlock(blocks, kBoxSwShSv);
    auto* partyBlock = SwishCrypto::findBlock(blocks, partyKeyFor(game));
    const auto* layout = SwishCrypto::findBlock(blocks, kBoxLayout);
    if (!boxBlock || boxBlock->type != ScType::Object) {
        result.message = "Box SCBlock missing";
        return result;
    }

    applyBoxNames(layout, result.boxes);

    const Generation gen = genFor(game);
    constexpr std::size_t kSlot = poke_crypto8::kPartySize;
    const std::size_t expectBoxes = boxesN * kDefaultBoxSlots * kSlot;
    if (boxBlock->data.size() < expectBoxes) {
        result.message = "Box block too small (" + std::to_string(boxBlock->data.size()) + ")";
        return result;
    }

    for (std::size_t b = 0; b < boxesN; ++b) {
        for (std::size_t s = 0; s < kDefaultBoxSlots; ++s) {
            const std::size_t off = (b * kDefaultBoxSlots + s) * kSlot;
            auto mon = decodePk8Party(boxBlock->data.data() + off, kSlot, gen);
            result.boxes[b].slot(s) = std::move(mon);
        }
    }

    if (partyBlock && partyBlock->type == ScType::Object) {
        const std::size_t partyBytes = 6 * kSlot;
        if (partyBlock->data.size() >= partyBytes) {
            int count = 6;
            if (partyBlock->data.size() > partyBytes) {
                count = std::min<int>(6, partyBlock->data[partyBytes]);
            }
            for (int i = 0; i < 6; ++i) {
                if (i < count) {
                    result.party.slot(std::size_t(i)) =
                        decodePk8Party(partyBlock->data.data() + std::size_t(i) * kSlot, kSlot, gen);
                }
            }
        }
    }

    result.ok = true;
    result.parseImplemented = true;
    result.message = "Parsed " + std::to_string(blocks.size()) + " SCBlocks for " +
                     std::string(gameDisplayName(game));
    return result;
}

bool serializeSwish(GameId game,
                    const std::vector<uint8_t>& original,
                    const Party& party,
                    const std::vector<Box>& boxes,
                    std::vector<uint8_t>& out,
                    std::string* err) {
    std::vector<ScBlock> blocks;
    if (!SwishCrypto::decrypt(original, blocks, err)) {
        return false;
    }
    auto* boxBlock = SwishCrypto::findBlock(blocks, kBoxSwShSv);
    auto* partyBlock = SwishCrypto::findBlock(blocks, partyKeyFor(game));
    if (!boxBlock) {
        if (err) {
            *err = "Box SCBlock missing on write";
        }
        return false;
    }

    constexpr std::size_t kSlot = poke_crypto8::kPartySize;
    const std::size_t boxesN = switchBoxCountFor(game);
    const std::size_t expectBoxes = boxesN * kDefaultBoxSlots * kSlot;
    if (boxBlock->data.size() < expectBoxes) {
        boxBlock->data.resize(expectBoxes, 0);
    }

    for (std::size_t b = 0; b < boxesN && b < boxes.size(); ++b) {
        for (std::size_t s = 0; s < kDefaultBoxSlots && s < boxes[b].size(); ++s) {
            const auto& mon = boxes[b].slot(s);
            const std::size_t off = (b * kDefaultBoxSlots + s) * kSlot;
            if (mon.empty()) {
                std::fill(boxBlock->data.begin() + static_cast<std::ptrdiff_t>(off),
                          boxBlock->data.begin() + static_cast<std::ptrdiff_t>(off + kSlot), 0);
                continue;
            }
            std::vector<uint8_t> enc;
            if (!patchAndEncryptPk8(mon, enc, err)) {
                return false;
            }
            std::memcpy(boxBlock->data.data() + off, enc.data(), kSlot);
        }
    }

    if (partyBlock) {
        const std::size_t partyBytes = 6 * kSlot;
        if (partyBlock->data.size() < partyBytes + 1) {
            partyBlock->data.resize(partyBytes + 1, 0);
        }
        int occupied = 0;
        for (std::size_t i = 0; i < 6; ++i) {
            const auto& mon = party.slot(i);
            if (mon.empty()) {
                std::fill(partyBlock->data.begin() + static_cast<std::ptrdiff_t>(i * kSlot),
                          partyBlock->data.begin() + static_cast<std::ptrdiff_t>((i + 1) * kSlot),
                          0);
                continue;
            }
            std::vector<uint8_t> enc;
            if (!patchAndEncryptPk8(mon, enc, err)) {
                return false;
            }
            std::memcpy(partyBlock->data.data() + i * kSlot, enc.data(), kSlot);
            occupied = int(i) + 1;
        }
        partyBlock->data[partyBytes] = uint8_t(occupied);
    }

    return SwishCrypto::encrypt(blocks, out, err);
}

}  // namespace

std::size_t switchBoxCountFor(GameId game) {
    switch (game) {
        case GameId::Sword:
        case GameId::Shield:
            return 32;
        case GameId::BrilliantDiamond:
        case GameId::ShiningPearl:
            return 40;
        case GameId::LegendsArceus:
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

    if (usesSwishPk8(game)) {
        return parseSwish(game, data);
    }

    // BDSP / LA / others: mount OK, entity parse still limited.
    result.ok = true;
    result.parseImplemented = false;
    result.message =
        "Mounted " + std::to_string(data.size()) + " bytes for " +
        std::string(gameDisplayName(game)) +
        ". Full Pokémon parse for this title is not implemented yet — empty boxes shown.";
    return result;
}

bool serializeSwitchSave(GameId game,
                         const std::vector<uint8_t>& original,
                         const Party& party,
                         const std::vector<Box>& boxes,
                         std::vector<uint8_t>& out,
                         std::string* err) {
    if (original.empty()) {
        if (err) {
            *err = "No original save buffer to write";
        }
        return false;
    }

    if (usesSwishPk8(game)) {
        return serializeSwish(game, original, party, boxes, out, err);
    }

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
                "Cannot write Pokémon into this Switch title until its parser/serializer "
                "is implemented.";
        }
        return false;
    }
    out = original;
    return true;
}

}  // namespace pkhub
