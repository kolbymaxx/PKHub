#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pkhub/backends/RawSaveBackend.hpp"
#include "pkhub/core/pokemon/GameId.hpp"

namespace pkhub {

/// Result of content + path probing for a raw save file or dump.
struct SaveProbeResult {
    RawSaveFormat format = RawSaveFormat::Unknown;
    GameId game = GameId::Unknown;
    Generation generation = Generation::Unknown;
    bool formatSupported = false;
    std::string formatHint;          // short UI chip, e.g. "GBA · Emerald"
    std::string unsupportedReason;
    std::string displayName;         // preferred list title
    int confidence = 0;              // 0–100
    bool isSwishDump = false;        // official Switch `main` blob (SwSh/SV)
};

/// Soft GameId guess from path / ROM basename tokens (never sole authority).
GameId guessGameFromPath(const std::string& path);

/// Classify bytes + path: Gen 3 section magic, size bands, SwishCrypto hash, path hints.
SaveProbeResult probeSaveBytes(const std::string& path, const std::vector<uint8_t>& data);

}  // namespace pkhub
