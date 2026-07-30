#include "pkhub/backends/SaveDiscovery.hpp"
#include "pkhub/backends/RawSaveBackend.hpp"
#include "pkhub/backends/SwitchSaveBackend.hpp"
#include "pkhub/platform/TitleIds.hpp"

namespace pkhub {

SaveBackendPtr SwitchSaveBackendFactory::create(const DetectedSave& detected) const {
    if (!detected.isSwitchOfficial || detected.titleId == 0) {
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
    } kGames[] = {
        {GameId::Sword, title_ids::Sword},
        {GameId::Shield, title_ids::Shield},
        {GameId::BrilliantDiamond, title_ids::BrilliantDiamond},
        {GameId::ShiningPearl, title_ids::ShiningPearl},
        {GameId::LegendsArceus, title_ids::LegendsArceus},
        {GameId::Scarlet, title_ids::Scarlet},
        {GameId::Violet, title_ids::Violet},
    };
    for (const auto& g : kGames) {
        DetectedSave d;
        d.game = g.game;
        d.generation = generationFor(g.game);
        d.displayName = gameDisplayName(g.game);
        d.titleId = g.tid;
        d.isSwitchOfficial = true;
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
