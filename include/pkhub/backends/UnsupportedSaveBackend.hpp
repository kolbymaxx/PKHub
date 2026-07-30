#pragma once

#include <memory>
#include <string>

#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/core/save/ISaveBackend.hpp"

namespace pkhub {

/**
 * Placeholder for titles whose save format is not yet documented
 * (e.g. Legends: Z-A in Phase 1). Appears in the game list but cannot open.
 */
class UnsupportedSaveBackend : public ISaveBackend {
public:
    UnsupportedSaveBackend(GameId game, std::string reason);

    SaveOpenStatus open() override;
    void close() override {}
    bool isOpen() const override { return false; }
    SaveOpenStatus reload() override;
    SaveOpenStatus commit() override;
    bool isDirty() const override { return false; }
    void markClean() override {}

    std::string displayName() const override;
    Generation generation() const override;
    GameId gameId() const override;
    std::size_t boxCount() const override { return 0; }
    Box& box(std::size_t) override;
    const Box& box(std::size_t) const override;
    Party& party() override;
    const Party& party() const override;
    bool isReadOnly() const override { return true; }

    const std::string& reason() const { return reason_; }

private:
    GameId game_;
    std::string reason_;
};

inline SaveBackendPtr makeLegendsZAStub() {
    return std::make_unique<UnsupportedSaveBackend>(
        GameId::LegendsZA,
        "Pokémon Legends: Z-A save format is not yet documented. "
        "Support will be added when the structure is known.");
}

}  // namespace pkhub
