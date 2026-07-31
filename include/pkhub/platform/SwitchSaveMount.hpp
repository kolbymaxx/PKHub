#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pkhub/platform/SaveAccess.hpp"

namespace pkhub {

enum class MountStatus {
    Ok = 0,
    NotAvailable,     // desktop without fixture, or APIs missing
    TitleMismatch,    // title override running under different title
    UserRequired,     // FsSaveData needs a user id
    PermissionDenied,
    NotFound,
    IoError,
};

struct SwitchMountResult {
    MountStatus status = MountStatus::NotAvailable;
    SaveAccessMode modeUsed = SaveAccessMode::Auto;
    std::string message;
    std::string saveFileName = "main";  // relative name inside save FS
    std::vector<uint8_t> data;
    bool writable = false;
};

/**
 * Mount / read / write official Switch title save data.
 * Priority in Auto: title override → fsOpen_SaveData.
 * On desktop: loads/saves fixture at sdmc:/switch/PKHub/switch_saves/<titleId>/main
 */
class SwitchSaveMount {
public:
    SwitchMountResult mountAndRead(const SwitchMountRequest& req);
    SwitchMountResult write(const SwitchMountRequest& req,
                            SaveAccessMode modeUsed,
                            const std::string& saveFileName,
                            const std::vector<uint8_t>& data);
    void unmount();

    bool isMounted() const { return mounted_; }
    SaveAccessMode lastMode() const { return lastMode_; }

    /// True when process appears to be running under title override for titleId.
    static bool isTitleOverrideFor(uint64_t titleId);

    /// Desktop/host fixture path (logical sdmc path).
    static std::string desktopFixturePath(uint64_t titleId, const std::string& fileName = "main");

private:
    SwitchMountResult mountTitleOverride(uint64_t titleId);
    SwitchMountResult mountFsSaveData(uint64_t titleId, const SwitchUserId& user);
    SwitchMountResult readSaveFile(const std::string& devicePrefix, const std::string& fileName);
    SwitchMountResult writeSaveFile(const std::string& devicePrefix,
                                    const std::string& fileName,
                                    const std::vector<uint8_t>& data);

    bool mounted_ = false;
    SaveAccessMode lastMode_ = SaveAccessMode::Auto;
    std::string mountName_;  // fsdev mount name when on device
};

const char* mountStatusLabel(MountStatus status);

}  // namespace pkhub
