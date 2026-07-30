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
    bool formatSupported = true;  // false → UnsupportedSaveBackend stub
    std::string unsupportedReason;
    std::string formatHint;  // e.g. "GBA", "NDS", "DSV"
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

/// Default RetroArch / Checkpoint-style roots on Switch SD.
std::vector<std::string> defaultEmulatorSaveRoots();

std::vector<DetectedSave> scanKnownSwitchTitles();

/// Walk roots for .sav / .srm / .dsv (and common 3DS dump extensions later).
std::vector<DetectedSave> scanRetroArchSaves(const std::vector<std::string>& roots = {});

/// Inspect a single file path; nullopt if missing / not a supported save extension.
std::optional<DetectedSave> detectRawSaveFile(const std::string& path);

}  // namespace pkhub
