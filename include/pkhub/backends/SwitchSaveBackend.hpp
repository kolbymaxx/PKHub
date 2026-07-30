#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/core/save/ISaveBackend.hpp"

namespace pkhub {

/// Switch account UID (matches libnx AccountUid layout conceptually).
struct SwitchUserId {
    uint64_t uid[2]{0, 0};
};

/**
 * Official Switch title saves via libnx.
 * Skeleton — mount/parse wired in Phase 1 implementation.
 */
class SwitchSaveBackend : public ISaveBackend {
public:
    SwitchSaveBackend(GameId game, uint64_t titleId, SwitchUserId userId = {});

    SaveOpenStatus open() override;
    void close() override;
    bool isOpen() const override { return open_; }
    SaveOpenStatus reload() override;
    SaveOpenStatus commit() override;
    bool isDirty() const override { return dirty_; }
    void markClean() override { dirty_ = false; }

    std::string displayName() const override;
    Generation generation() const override;
    GameId gameId() const override;
    std::size_t boxCount() const override;
    Box& box(std::size_t index) override;
    const Box& box(std::size_t index) const override;
    Party& party() override;
    const Party& party() const override;

private:
    GameId game_;
    uint64_t titleId_;
    SwitchUserId userId_;
    bool open_ = false;
    bool dirty_ = false;
    std::vector<Box> boxes_;
    Party party_;
};

}  // namespace pkhub
