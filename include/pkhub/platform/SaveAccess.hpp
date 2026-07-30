#pragma once

#include <cstdint>
#include <string>

namespace pkhub {

/**
 * How official Switch saves are mounted.
 * Prefer title override when available; fall back to fsOpen_SaveData + user picker.
 */
enum class SaveAccessMode : uint8_t {
    /// Hold R (or configured) while launching the Pokémon title, then open PKHub.
    TitleOverride = 0,
    /// Mount via libnx fsOpen_SaveData / equivalent with title ID + user UID.
    FsSaveData,
    /// Auto: try title-override context first, then FsSaveData.
    Auto,
};

const char* saveAccessModeLabel(SaveAccessMode mode);

struct SwitchMountRequest {
    uint64_t titleId = 0;
    SaveAccessMode mode = SaveAccessMode::Auto;
    /// When using FsSaveData; zero means prompt / use selected user.
    uint64_t userUid[2]{0, 0};
};

}  // namespace pkhub
