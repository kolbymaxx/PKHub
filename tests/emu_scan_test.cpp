#include "pkhub/backends/SaveDiscovery.hpp"
#include "pkhub/backends/SaveProbe.hpp"
#include "pkhub/backends/raw/GbaGen3.hpp"
#include "pkhub/core/fs/FileBrowser.hpp"
#include "pkhub/core/fs/Paths.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

void writeBytes(const std::string& logicalPath, const std::vector<uint8_t>& data) {
    const std::string resolved = pkhub::fs::resolvePath(logicalPath);
    std::filesystem::create_directories(std::filesystem::path(resolved).parent_path());
    std::ofstream out(resolved, std::ios::binary | std::ios::trunc);
    assert(out);
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
}

void write16(uint8_t* p, uint16_t v) {
    p[0] = uint8_t(v);
    p[1] = uint8_t(v >> 8);
}

void write32(uint8_t* p, uint32_t v) {
    p[0] = uint8_t(v);
    p[1] = uint8_t(v >> 8);
    p[2] = uint8_t(v >> 16);
    p[3] = uint8_t(v >> 24);
}

/// Minimal dual-slot Gen 3 save with Emerald-ish trainer code.
std::vector<uint8_t> makeValidGbaSave() {
    using namespace pkhub::gba;
    std::vector<uint8_t> data(kGbaSaveSize, 0);
    for (int slot = 0; slot < 2; ++slot) {
        uint8_t* slotBase = data.data() + std::size_t(slot) * kSlotSize;
        const uint32_t idx = slot == 0 ? 2u : 1u;
        for (uint16_t id = 0; id < kSectionsPerSlot; ++id) {
            uint8_t* sec = slotBase + std::size_t(id) * kSectionSize;
            if (id == 0) {
                write32(sec + 0xAC, 0x12345678u);  // Emerald security key-ish
            }
            write16(sec + 0x0FF4, id);
            write16(sec + 0x0FF6, 0);
            write32(sec + 0x0FF8, kSectionSignature);
            write32(sec + 0x0FFC, idx);
        }
    }
    return data;
}

}  // namespace

int main() {
    const std::string gbaPath = "/retroarch/saves/pokemon_emerald.sav";
    const std::string ndsPath = "/retroarch/cores/savefiles/heartgold.sav";
    const std::string junkPath = "/retroarch/saves/notes.txt";
    const std::string blankGba = "/retroarch/saves/blank128.sav";

    writeBytes(gbaPath, makeValidGbaSave());

    std::vector<uint8_t> nds(0x80000, 0xAB);
    writeBytes(ndsPath, nds);
    writeBytes(junkPath, {1, 2, 3, 4});
    writeBytes(blankGba, std::vector<uint8_t>(0x20000, 0));

    assert(pkhub::fs::isDirectory("/retroarch/saves"));
    auto listing = pkhub::fs::listDirectory("/retroarch/saves");
    assert(!listing.empty());

    auto files = pkhub::fs::findFilesWithExtensions(
        "/retroarch", {"sav", "srm", "dsv"}, /*maxDepth=*/3, /*maxFiles=*/50);
    assert(files.size() >= 2);

    // Path hint alone
    assert(pkhub::guessGameFromPath("/foo/Pokemon - Emerald Version.sav") ==
           pkhub::GameId::Emerald);
    assert(pkhub::guessGameFromPath("sdmc:/saves/firered.srm") == pkhub::GameId::FireRed);
    assert(pkhub::guessGameFromPath("heartgold.sav") == pkhub::GameId::HeartGold);

    auto gbaDet = pkhub::detectRawSaveFile(gbaPath);
    assert(gbaDet.has_value());
    assert(gbaDet->formatSupported == true);
    assert(gbaDet->game == pkhub::GameId::Emerald);
    assert(gbaDet->formatHint.find("GBA") != std::string::npos);

    // Zero-filled 128KB is no longer treated as a valid Gen 3 save.
    auto blankDet = pkhub::detectRawSaveFile(blankGba);
    assert(blankDet.has_value());
    assert(blankDet->formatSupported == false);

    auto ndsDet = pkhub::detectRawSaveFile(ndsPath);
    assert(ndsDet.has_value());
    assert(ndsDet->formatSupported == false);
    assert(ndsDet->game == pkhub::GameId::HeartGold);
    assert(ndsDet->formatHint.find("HeartGold") != std::string::npos ||
           ndsDet->formatHint.find("NDS") != std::string::npos);

    assert(!pkhub::detectRawSaveFile(junkPath).has_value());

    auto scanned = pkhub::scanRetroArchSaves({"/retroarch/saves", "/retroarch/cores/savefiles"});
    assert(scanned.size() >= 2);
    // Supported GBA sorted first
    assert(scanned.front().formatSupported == true);
    assert(scanned.front().formatHint.rfind("GBA", 0) == 0);

    auto defaults = pkhub::defaultEmulatorSaveRoots();
    assert(!defaults.empty());

    std::puts("emu_scan_test OK");
    return 0;
}
