#include "pkhub/backends/SaveDiscovery.hpp"
#include "pkhub/backends/RawSaveBackend.hpp"
#include "pkhub/backends/SwitchSaveBackend.hpp"
#include "pkhub/backends/UnsupportedSaveBackend.hpp"
#include "pkhub/core/fs/FileBrowser.hpp"
#include "pkhub/core/fs/Paths.hpp"
#include "pkhub/platform/TitleIds.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>

namespace pkhub {
namespace {

std::string basenameOf(const std::string& path) {
    const auto pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

std::string formatHintFor(RawSaveFormat fmt) {
    switch (fmt) {
        case RawSaveFormat::GbaSav: return "GBA";
        case RawSaveFormat::NdsSav: return "NDS";
        case RawSaveFormat::NdsDsv: return "DSV";
        case RawSaveFormat::CtrSave: return "3DS";
        default: return "RAW";
    }
}

Generation generationHintFor(RawSaveFormat fmt) {
    switch (fmt) {
        case RawSaveFormat::GbaSav: return Generation::Gen3;
        case RawSaveFormat::NdsSav:
        case RawSaveFormat::NdsDsv: return Generation::Gen4;  // refined on open
        case RawSaveFormat::CtrSave: return Generation::Gen6;
        default: return Generation::Unknown;
    }
}

}  // namespace

SaveBackendPtr SwitchSaveBackendFactory::create(const DetectedSave& detected) const {
    if (!detected.isSwitchOfficial) {
        return nullptr;
    }
    if (!detected.formatSupported) {
        return std::make_unique<UnsupportedSaveBackend>(
            detected.game,
            detected.unsupportedReason.empty()
                ? std::string("Save format is not yet documented.")
                : detected.unsupportedReason);
    }
    if (detected.titleId == 0) {
        return nullptr;
    }
    return std::make_unique<SwitchSaveBackend>(detected.game, detected.titleId);
}

SaveBackendPtr RawSaveBackendFactory::create(const DetectedSave& detected) const {
    if (detected.path.empty()) {
        return nullptr;
    }
    return std::make_unique<RawSaveBackend>(detected.path);
}

std::vector<std::string> defaultEmulatorSaveRoots() {
    return {
        "sdmc:/retroarch/cores/savefiles",
        "sdmc:/retroarch/saves",
        "/retroarch/cores/savefiles",
        "/retroarch/saves",
        "sdmc:/switch/Checkpoint/saves",
        "/switch/Checkpoint/saves",
        "sdmc:/emu",
        "/emu",
    };
}

std::vector<DetectedSave> scanKnownSwitchTitles() {
    std::vector<DetectedSave> out;
    const struct {
        GameId game;
        uint64_t tid;
        bool supported;
    } kGames[] = {
        {GameId::Sword, title_ids::Sword, true},
        {GameId::Shield, title_ids::Shield, true},
        {GameId::BrilliantDiamond, title_ids::BrilliantDiamond, true},
        {GameId::ShiningPearl, title_ids::ShiningPearl, true},
        {GameId::LegendsArceus, title_ids::LegendsArceus, true},
        {GameId::Scarlet, title_ids::Scarlet, true},
        {GameId::Violet, title_ids::Violet, true},
        {GameId::LegendsZA, title_ids::LegendsZA, false},
    };
    for (const auto& g : kGames) {
        DetectedSave d;
        d.game = g.game;
        d.generation = generationFor(g.game);
        d.displayName = gameDisplayName(g.game);
        d.titleId = g.tid;
        d.isSwitchOfficial = true;
        d.formatSupported = g.supported;
        if (!g.supported) {
            d.unsupportedReason =
                "Pokémon Legends: Z-A save format is not yet documented. "
                "Support will be added when the structure is known.";
        }
        out.push_back(std::move(d));
    }
    return out;
}

std::optional<DetectedSave> detectRawSaveFile(const std::string& path) {
    if (path.empty() || !fs::isRegularFile(path)) {
        return std::nullopt;
    }
    const std::string ext = fs::fileExtension(path);
    if (ext != "sav" && ext != "srm" && ext != "dsv") {
        return std::nullopt;
    }

    std::ifstream in(fs::resolvePath(path), std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    const RawSaveFormat fmt = detectRawFormat(path, data);

    DetectedSave d;
    d.path = path;
    d.displayName = basenameOf(path);
    d.isSwitchOfficial = false;
    d.formatHint = formatHintFor(fmt);
    d.generation = generationHintFor(fmt);
    d.game = GameId::Unknown;
    d.formatSupported = (fmt == RawSaveFormat::GbaSav);  // Phase 1: GBA open/write ready
    if (!d.formatSupported) {
        d.unsupportedReason = "Parser for this raw format is not implemented yet.";
    }
    return d;
}

std::vector<DetectedSave> scanRetroArchSaves(const std::vector<std::string>& roots) {
    const std::vector<std::string> useRoots =
        roots.empty() ? defaultEmulatorSaveRoots() : roots;

    std::set<std::string> seenResolved;
    std::vector<DetectedSave> out;

    const std::vector<std::string> exts = {"sav", "srm", "dsv"};
    for (const auto& root : useRoots) {
        if (!fs::isDirectory(root)) {
            continue;
        }
        auto files = fs::findFilesWithExtensions(root, exts, /*maxDepth=*/5, /*maxFiles=*/300);
        for (const auto& file : files) {
            // Dedupe sdmc:/foo vs /foo on desktop (same resolved path).
            if (!seenResolved.insert(fs::resolvePath(file)).second) {
                continue;
            }
            if (auto det = detectRawSaveFile(file)) {
                out.push_back(std::move(*det));
            }
        }
    }

    // GBA (.sav/.srm that look like GBA) first, then others.
    std::sort(out.begin(), out.end(), [](const DetectedSave& a, const DetectedSave& b) {
        const int ra = (a.formatHint == "GBA") ? 0 : 1;
        const int rb = (b.formatHint == "GBA") ? 0 : 1;
        if (ra != rb) {
            return ra < rb;
        }
        return a.path < b.path;
    });
    return out;
}

}  // namespace pkhub
