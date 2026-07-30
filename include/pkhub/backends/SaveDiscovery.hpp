#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/core/save/ISaveBackend.hpp"

namespace pkhub {

struct DetectedSave {
    GameId game = GameId::Unknown;
    Generation generation = Generation::Unknown;
    std::string path;          // empty for mounted Switch saves
    std::string displayName;
    uint64_t titleId = 0;      // Switch only
    bool isSwitchOfficial = false;
};

class ISaveBackendFactory {
public:
    virtual ~ISaveBackendFactory() = default;
    virtual SaveBackendPtr create(const DetectedSave& detected) const = 0;
};

/// Official Switch titles via libnx save APIs.
class SwitchSaveBackendFactory : public ISaveBackendFactory {
public:
    SaveBackendPtr create(const DetectedSave& detected) const override;
};

/// Emulator / raw dump files.
class RawSaveBackendFactory : public ISaveBackendFactory {
public:
    SaveBackendPtr create(const DetectedSave& detected) const override;
};

std::vector<DetectedSave> scanKnownSwitchTitles();
std::vector<DetectedSave> scanRetroArchSaves(const std::vector<std::string>& roots);
std::optional<DetectedSave> detectRawSaveFile(const std::string& path);

}  // namespace pkhub
