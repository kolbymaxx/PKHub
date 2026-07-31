#pragma once

#include <cstdint>
#include <string>
#include <vector>

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

/// Switch account UID (matches libnx AccountUid layout conceptually).
struct SwitchUserId {
    uint64_t uid[2]{0, 0};

    bool isZero() const { return uid[0] == 0 && uid[1] == 0; }
};

struct SwitchMountRequest {
    uint64_t titleId = 0;
    SaveAccessMode mode = SaveAccessMode::Auto;
    SwitchUserId user{};
    std::string saveFileName = "main";
};

struct SwitchUserInfo {
    SwitchUserId id{};
    std::string nickname;
};

/// Enumerate local Switch users for FsSaveData mounts (empty on desktop).
std::vector<SwitchUserInfo> listSwitchUsers();

}  // namespace pkhub
