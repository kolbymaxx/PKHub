#include "pkhub/backends/RawSaveBackend.hpp"
#include "pkhub/backends/raw/GbaGen3.hpp"
#include "pkhub/core/fs/Paths.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

using pkhub::Box;
using pkhub::GameId;
using pkhub::Party;
using pkhub::Pokemon;
using pkhub::RawSaveBackend;
using pkhub::gba::decodeBoxMon;
using pkhub::gba::decodePartyMon;
using pkhub::gba::encodeBoxMon;
using pkhub::gba::encodePartyMon;
using pkhub::gba::kBoxCount;
using pkhub::gba::kBoxMonSize;
using pkhub::gba::kGbaSaveSize;
using pkhub::gba::kPartyMonSize;
using pkhub::gba::kSectionSignature;
using pkhub::gba::kSectionSize;
using pkhub::gba::kSectionsPerSlot;
using pkhub::gba::kSlotSize;
using pkhub::gba::kSlotsPerBox;
using pkhub::gba::parseSave;
using pkhub::gba::writeSave;

void write16(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
}

void write32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFF);
}

uint32_t read32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

uint16_t read16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

constexpr uint16_t kSectionChecksumSize[14] = {
    0xF24, 0xF80, 0xF80, 0xF80, 0xF08, 0xF80, 0xF80,
    0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0x7D0,
};

uint16_t sectionChecksum(const uint8_t* section, uint16_t byteSize) {
    uint32_t sum = 0;
    for (std::size_t i = 0; i < byteSize / 4; ++i) {
        sum += read32(section + i * 4);
    }
    return static_cast<uint16_t>((sum + (sum >> 16)) & 0xFFFF);
}

void finalizeSection(uint8_t* section, uint16_t id, uint32_t saveIndex) {
    const uint16_t chk = sectionChecksum(section, kSectionChecksumSize[id]);
    write16(section + 0x0FF4, id);
    write16(section + 0x0FF6, chk);
    write32(section + 0x0FF8, kSectionSignature);
    write32(section + 0x0FFC, saveIndex);
}

/// Build a minimal dual-slot Emerald-like save with empty party/PC.
std::vector<uint8_t> makeMinimalSave(uint32_t saveIndex = 1) {
    std::vector<uint8_t> data(kGbaSaveSize, 0);
    for (int slot = 0; slot < 2; ++slot) {
        uint8_t* slotBase = data.data() + static_cast<std::size_t>(slot) * kSlotSize;
        const uint32_t idx = (slot == 0) ? saveIndex : (saveIndex > 0 ? saveIndex - 1 : 0);
        for (uint16_t id = 0; id < kSectionsPerSlot; ++id) {
            uint8_t* sec = slotBase + static_cast<std::size_t>(id) * kSectionSize;
            // Trainer: mark Emerald-ish code at 0xAC
            if (id == 0) {
                write32(sec + 0xAC, 0x12345678u);
            }
            finalizeSection(sec, id, idx);
        }
    }
    return data;
}

Pokemon makePika(bool shiny, uint8_t level) {
    Pokemon src;
    src.species = 25;
    src.pid = 0x0A0B0C0D;
    src.otId = 12345;
    src.otSecretId = 54321;
    src.level = level;
    src.exp = 125000;
    src.friendship = 200;
    src.isShiny = shiny;
    src.ivs = {31, 30, 29, 28, 27, 26};
    src.evs = {10, 20, 30, 40, 50, 60};
    src.moves = {84, 85, 86, 87};
    src.pp = {30, 15, 20, 10};
    src.abilitySlot = 1;
    src.metLevel = 5;
    src.ball = 4;
    return src;
}

bool checksumsValid(const std::vector<uint8_t>& data, std::size_t slotOff, uint32_t expectIndex) {
    const uint8_t* slotBase = data.data() + slotOff;
    for (uint16_t id = 0; id < kSectionsPerSlot; ++id) {
        // Find section with this id
        const uint8_t* found = nullptr;
        for (std::size_t i = 0; i < kSectionsPerSlot; ++i) {
            const uint8_t* sec = slotBase + i * kSectionSize;
            if (read16(sec + 0x0FF4) == id && read32(sec + 0x0FF8) == kSectionSignature) {
                found = sec;
                break;
            }
        }
        if (!found) {
            return false;
        }
        if (read32(found + 0x0FFC) != expectIndex) {
            return false;
        }
        const uint16_t expect = sectionChecksum(found, kSectionChecksumSize[id]);
        if (read16(found + 0x0FF6) != expect) {
            return false;
        }
    }
    return true;
}

}  // namespace

int main() {
    // encodePartyMon level trailer
    {
        Pokemon p = makePika(true, 42);
        std::array<uint8_t, 100> blob{};
        assert(encodePartyMon(p, blob.data()));
        assert(blob[0x54] == 42);
        Pokemon out;
        assert(decodePartyMon(blob.data(), out));
        assert(out.species == 25);
        assert(out.level == 42);
        assert(out.isShiny);
    }

    // Full writeSave → parseSave round-trip
    {
        auto data = makeMinimalSave(3);
        Party party;
        std::vector<Box> boxes(kBoxCount, Box{kSlotsPerBox});
        for (std::size_t i = 0; i < boxes.size(); ++i) {
            boxes[i].setName("Box " + std::to_string(i + 1));
        }

        party.slot(0) = makePika(true, 50);
        boxes[0].slot(0) = makePika(false, 20);
        boxes[0].slot(0).species = 1;  // Bulbasaur
        boxes[0].slot(0).pid = 0x55667788;
        boxes[0].slot(0).isShiny = false;

        std::string err;
        assert(writeSave(data, GameId::Emerald, party, boxes, &err));
        assert(err.empty());
        assert(checksumsValid(data, 0, 4));  // active was slot0 idx=3 → bumped to 4

        const auto parsed = parseSave(data);
        assert(parsed.ok);
        assert(parsed.party.slot(0).species == 25);
        assert(parsed.party.slot(0).isShiny == true);
        assert(parsed.party.slot(0).level == 50);
        assert(parsed.party.slot(0).moves[0] == 84);
        assert(parsed.party.slot(0).ivs.hp == 31);

        assert(parsed.boxes[0].slot(0).species == 1);
        assert(parsed.boxes[0].slot(0).isShiny == false);
        assert(parsed.boxes[0].slot(0).moves[1] == 85);
    }

    // RawSaveBackend::commit persists to disk and reloads
    {
        const std::string dir = "pkhub_data/test_gba_writeback";
        std::filesystem::create_directories(dir);
        const std::string path = dir + "/emerald.sav";

        auto data = makeMinimalSave(1);
        {
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            assert(out);
            out.write(reinterpret_cast<const char*>(data.data()),
                      static_cast<std::streamsize>(data.size()));
        }

        RawSaveBackend backend(path);
        auto st = backend.open();
        assert(st.result == pkhub::SaveOpenResult::Ok);

        backend.party().slot(0) = makePika(true, 33);
        backend.box(2).slot(5) = makePika(false, 12);
        backend.box(2).slot(5).species = 4;  // Charmander

        st = backend.commit();
        assert(st.result == pkhub::SaveOpenResult::Ok);

        RawSaveBackend reload(path);
        st = reload.open();
        assert(st.result == pkhub::SaveOpenResult::Ok);
        assert(reload.party().slot(0).species == 25);
        assert(reload.party().slot(0).level == 33);
        assert(reload.party().slot(0).isShiny == true);
        assert(reload.box(2).slot(5).species == 4);
        assert(reload.box(2).slot(5).isShiny == false);
    }

    std::puts("gba_writeback_test OK");
    return 0;
}
