#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/core/save/ISaveBackend.hpp"

namespace pkhub {

enum class RawSaveFormat {
    Unknown,
    GbaSav,
    NdsSav,
    NdsDsv,
    CtrSave,  // 3DS dumps
};

/**
 * Emulator / raw file saves (.sav, .dsv, .srm, 3DS dumps).
 * Skeleton — format detection + gen parsers in Phase 1.
 */
class RawSaveBackend : public ISaveBackend {
public:
    explicit RawSaveBackend(std::string path);

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

    RawSaveFormat format() const { return format_; }
    const std::string& path() const { return path_; }

private:
    std::string path_;
    RawSaveFormat format_ = RawSaveFormat::Unknown;
    GameId game_ = GameId::Unknown;
    bool open_ = false;
    bool dirty_ = false;
    std::vector<uint8_t> raw_;
    std::vector<Box> boxes_;
    Party party_;
};

RawSaveFormat detectRawFormat(const std::string& path, const std::vector<uint8_t>& data);

}  // namespace pkhub
