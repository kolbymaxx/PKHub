#include "pkhub/backends/SaveDiscovery.hpp"
#include "pkhub/backends/RawSaveBackend.hpp"
#include "pkhub/backends/SwitchSaveBackend.hpp"
#include "pkhub/backends/UnsupportedSaveBackend.hpp"
#include "pkhub/platform/TitleIds.hpp"

namespace pkhub {

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

std::vector<DetectedSave> scanRetroArchSaves(const std::vector<std::string>& roots) {
    (void)roots;
    // TODO(phase1): directory walk for .sav/.srm/.dsv
    return {};
}

std::optional<DetectedSave> detectRawSaveFile(const std::string& path) {
    DetectedSave d;
    d.path = path;
    d.displayName = path;
    d.isSwitchOfficial = false;
    // Refined after reading file header/size.
    d.generation = Generation::Unknown;
    d.game = GameId::Unknown;
    return d;
}

}  // namespace pkhub
