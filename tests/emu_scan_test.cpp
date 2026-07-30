#include "pkhub/backends/SaveDiscovery.hpp"
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

}  // namespace

int main() {
    // Desktop maps /retroarch/... under ./pkhub_data
    const std::string gbaPath = "/retroarch/saves/pokemon_emerald.sav";
    const std::string ndsPath = "/retroarch/cores/savefiles/heartgold.sav";
    const std::string junkPath = "/retroarch/saves/notes.txt";

    std::vector<uint8_t> gba(0x20000, 0);
    // Minimal section footers so detectRawFormat still classifies by size as GBA.
    writeBytes(gbaPath, gba);

    std::vector<uint8_t> nds(0x80000, 0xAB);  // typical larger NDS save
    writeBytes(ndsPath, nds);
    writeBytes(junkPath, {1, 2, 3, 4});

    assert(pkhub::fs::isDirectory("/retroarch/saves"));
    auto listing = pkhub::fs::listDirectory("/retroarch/saves");
    assert(!listing.empty());

    auto files = pkhub::fs::findFilesWithExtensions(
        "/retroarch", {"sav", "srm", "dsv"}, /*maxDepth=*/3, /*maxFiles=*/50);
    assert(files.size() >= 2);

    auto gbaDet = pkhub::detectRawSaveFile(gbaPath);
    assert(gbaDet.has_value());
    assert(gbaDet->formatHint == "GBA");
    assert(gbaDet->formatSupported == true);

    auto ndsDet = pkhub::detectRawSaveFile(ndsPath);
    assert(ndsDet.has_value());
    assert(ndsDet->formatSupported == false);  // NDS parser later

    assert(!pkhub::detectRawSaveFile(junkPath).has_value());

    auto scanned = pkhub::scanRetroArchSaves({"/retroarch/saves", "/retroarch/cores/savefiles"});
    assert(scanned.size() >= 2);
    // GBA sorted before non-GBA
    assert(scanned.front().formatHint == "GBA");

    auto defaults = pkhub::defaultEmulatorSaveRoots();
    assert(!defaults.empty());

    std::puts("emu_scan_test OK");
    return 0;
}
