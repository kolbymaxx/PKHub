#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/core/save/ISaveBackend.hpp"
#include "pkhub/platform/SaveAccess.hpp"
#include "pkhub/platform/SwitchSaveMount.hpp"

namespace pkhub {

/**
 * Official Switch title saves via libnx.
 * Prefer title-override mount; also support fsOpen_SaveData + user picker.
 */
class SwitchSaveBackend : public ISaveBackend {
public:
    SwitchSaveBackend(GameId game,
                      uint64_t titleId,
                      SwitchUserId userId = {},
                      SaveAccessMode accessMode = SaveAccessMode::Auto);

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

    SaveAccessMode accessMode() const { return accessMode_; }
    void setAccessMode(SaveAccessMode mode) { accessMode_ = mode; }
    SaveAccessMode modeUsed() const { return modeUsed_; }
    bool parseImplemented() const { return parseImplemented_; }
    const std::string& lastMountMessage() const { return mountMessage_; }

private:
    GameId game_;
    uint64_t titleId_;
    SwitchUserId userId_;
    SaveAccessMode accessMode_ = SaveAccessMode::Auto;
    SaveAccessMode modeUsed_ = SaveAccessMode::Auto;
    bool open_ = false;
    bool dirty_ = false;
    bool parseImplemented_ = false;
    std::string mountMessage_;
    std::string saveFileName_ = "main";
    std::vector<uint8_t> raw_;
    std::vector<Box> boxes_;
    Party party_;
    SwitchSaveMount mount_;
};

}  // namespace pkhub
