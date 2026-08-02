#include "pkhub/backends/SaveDiscovery.hpp"
#include "pkhub/backends/RawSaveBackend.hpp"
#include "pkhub/backends/SaveProbe.hpp"
#include "pkhub/backends/SwitchSaveBackend.hpp"
#include "pkhub/backends/UnsupportedSaveBackend.hpp"
#include "pkhub/core/fs/FileBrowser.hpp"
#include "pkhub/core/fs/Paths.hpp"
#include "pkhub/platform/TitleIds.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <set>

namespace pkhub {
namespace {

bool isMountOnlySwitchGame(GameId game) {
    switch (game) {
        case GameId::BrilliantDiamond:
        case GameId::ShiningPearl:
        case GameId::LegendsArceus:
            return true;
        default:
            return false;
    }
}

bool isPcReadySwitchGame(GameId game) {
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

int sortRank(const DetectedSave& d) {
    if (d.formatSupported && d.formatHint.rfind("GBA", 0) == 0) {
        return 0;
    }
    if (d.formatSupported) {
        return 1;
    }
    if (d.game != GameId::Unknown) {
        return 2;
    }
    return 3;
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
            d.formatHint = "Coming soon";
        } else if (isPcReadySwitchGame(g.game)) {
            d.formatHint = "Open PC";
        } else if (isMountOnlySwitchGame(g.game)) {
            d.formatHint = "Mount · boxes soon";
            d.unsupportedReason =
                "Save mounts, but entity parse needs clean-room offsets.";
        } else {
            d.formatHint = "Open PC";
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
    const std::string base = [&]() {
        const auto pos = path.find_last_of('/');
        return (pos == std::string::npos) ? path : path.substr(pos + 1);
    }();
    std::string baseLower = base;
    for (char& c : baseLower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    // Accept classic emu extensions + extensionless / .bin Switch `main` dumps.
    const bool okExt = (ext == "sav" || ext == "srm" || ext == "dsv" || ext == "bin" ||
                        ext.empty() || baseLower == "main");
    if (!okExt) {
        return std::nullopt;
    }

    std::ifstream in(fs::resolvePath(path), std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    if (data.empty()) {
        return std::nullopt;
    }

    const SaveProbeResult probe = probeSaveBytes(path, data);
    // Drop total unknowns with no useful hint (e.g. random .bin).
    if (probe.format == RawSaveFormat::Unknown && !probe.isSwishDump &&
        probe.confidence < 30 && probe.game == GameId::Unknown) {
        if (ext == "bin" || ext.empty()) {
            return std::nullopt;
        }
    }

    DetectedSave d;
    d.path = path;
    d.displayName = probe.displayName.empty() ? base : probe.displayName;
    d.isSwitchOfficial = false;
    d.formatHint = probe.formatHint;
    d.generation = probe.generation;
    d.game = probe.game;
    d.formatSupported = probe.formatSupported;
    d.unsupportedReason = probe.unsupportedReason;
    return d;
}

std::vector<DetectedSave> scanRetroArchSaves(const std::vector<std::string>& roots) {
    const std::vector<std::string> useRoots =
        roots.empty() ? defaultEmulatorSaveRoots() : roots;

    std::set<std::string> seenResolved;
    std::vector<DetectedSave> out;

    const std::vector<std::string> exts = {"sav", "srm", "dsv", "bin"};
    for (const auto& root : useRoots) {
        if (!fs::isDirectory(root)) {
            continue;
        }
        auto files = fs::findFilesWithExtensions(root, exts, /*maxDepth=*/5, /*maxFiles=*/300);
        for (const auto& file : files) {
            if (!seenResolved.insert(fs::resolvePath(file)).second) {
                continue;
            }
            if (auto det = detectRawSaveFile(file)) {
                out.push_back(std::move(*det));
            }
        }
    }

    std::sort(out.begin(), out.end(), [](const DetectedSave& a, const DetectedSave& b) {
        const int ra = sortRank(a);
        const int rb = sortRank(b);
        if (ra != rb) {
            return ra < rb;
        }
        return a.path < b.path;
    });
    return out;
}

}  // namespace pkhub
