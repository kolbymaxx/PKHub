#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/core/save/ISaveBackend.hpp"

namespace pkhub {

struct HubConfig {
    std::string rootPath = "sdmc:/switch/PKHub/hub";
    std::size_t defaultBoxCount = 50;
    std::size_t slotsPerBox = kDefaultBoxSlots;
};

/**
 * Persistent Hub Storage — first-class IBoxProvider.
 * Stores Pokémon independently of any game save for cross-game moves.
 */
class HubStorage : public IBoxProvider {
public:
    explicit HubStorage(HubConfig config = {});
    ~HubStorage() override;

    HubStorage(const HubStorage&) = delete;
    HubStorage& operator=(const HubStorage&) = delete;

    SaveOpenStatus openOrCreate();
    SaveOpenStatus reload();
    SaveOpenStatus commit();
    void close();

    bool isOpen() const { return open_; }
    bool isDirty() const { return dirty_; }

    // IBoxProvider
    std::string displayName() const override;
    Generation generation() const override;
    GameId gameId() const override;
    std::size_t boxCount() const override;
    Box& box(std::size_t index) override;
    const Box& box(std::size_t index) const override;
    Party& party() override;
    const Party& party() const override;

    // Hub-specific
    SaveOpenStatus addBox(const std::string& name = {});
    SaveOpenStatus removeBox(std::size_t index);

    const std::string& rootPath() const { return config_.rootPath; }

private:
    HubConfig config_;
    bool open_ = false;
    bool dirty_ = false;
    std::vector<Box> boxes_;
    Party party_;  // optional holding area; may be unused in UI
};

}  // namespace pkhub
