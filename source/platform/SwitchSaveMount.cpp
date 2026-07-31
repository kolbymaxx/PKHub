#include "pkhub/platform/SwitchSaveMount.hpp"

#include "pkhub/core/fs/Paths.hpp"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>

#if defined(__SWITCH__)
#include <switch.h>
#endif

namespace pkhub {
namespace {

#if defined(__SWITCH__)
constexpr const char* kFsdevName = "pkhubsave";
#endif

std::string titleIdHex(uint64_t titleId) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << titleId;
    return oss.str();
}

SwitchMountResult readHostFile(const std::string& hostPath, const std::string& logicalLabel) {
    SwitchMountResult out;
    std::ifstream in(hostPath, std::ios::binary);
    if (!in) {
        out.status = MountStatus::NotFound;
        out.message = "Save file not found: " + logicalLabel;
        return out;
    }
    out.data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    if (out.data.empty()) {
        out.status = MountStatus::IoError;
        out.message = "Save file empty: " + logicalLabel;
        return out;
    }
    out.status = MountStatus::Ok;
    out.writable = true;
    out.saveFileName = "main";
    out.message = "Read " + std::to_string(out.data.size()) + " bytes from " + logicalLabel;
    return out;
}

SwitchMountResult writeHostFile(const std::string& hostPath,
                                const std::string& logicalLabel,
                                const std::vector<uint8_t>& data) {
    SwitchMountResult out;
    std::ofstream o(hostPath, std::ios::binary | std::ios::trunc);
    if (!o) {
        out.status = MountStatus::IoError;
        out.message = "Cannot write " + logicalLabel;
        return out;
    }
    o.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
    if (!o) {
        out.status = MountStatus::IoError;
        out.message = "Write failed: " + logicalLabel;
        return out;
    }
    out.status = MountStatus::Ok;
    out.writable = true;
    out.saveFileName = "main";
    out.message = "Wrote " + std::to_string(data.size()) + " bytes to " + logicalLabel;
    return out;
}

}  // namespace

const char* mountStatusLabel(MountStatus status) {
    switch (status) {
        case MountStatus::Ok: return "OK";
        case MountStatus::NotAvailable: return "Not available";
        case MountStatus::TitleMismatch: return "Title mismatch";
        case MountStatus::UserRequired: return "User required";
        case MountStatus::PermissionDenied: return "Permission denied";
        case MountStatus::NotFound: return "Not found";
        case MountStatus::IoError: return "I/O error";
        default: return "Unknown";
    }
}

std::string SwitchSaveMount::desktopFixturePath(uint64_t titleId, const std::string& fileName) {
    return std::string(fs::kAppRoot) + "/switch_saves/" + titleIdHex(titleId) + "/" + fileName;
}

bool SwitchSaveMount::isTitleOverrideFor(uint64_t titleId) {
#if defined(__SWITCH__)
    u64 running = 0;
    if (R_SUCCEEDED(svcGetInfo(&running, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0))) {
        return running == titleId;
    }
    return false;
#else
    (void)titleId;
    if (const char* env = std::getenv("PKHUB_TITLE_OVERRIDE")) {
        return env[0] == '1' || env[0] == 'y' || env[0] == 'Y';
    }
    return false;
#endif
}

void SwitchSaveMount::unmount() {
#if defined(__SWITCH__)
    if (mounted_ && !mountName_.empty()) {
        fsdevUnmountDevice(mountName_.c_str());
    }
#endif
    mounted_ = false;
    mountName_.clear();
}

SwitchMountResult SwitchSaveMount::readSaveFile(const std::string& devicePrefix,
                                                const std::string& fileName) {
    return readHostFile(devicePrefix + fileName, devicePrefix + fileName);
}

SwitchMountResult SwitchSaveMount::writeSaveFile(const std::string& devicePrefix,
                                                 const std::string& fileName,
                                                 const std::vector<uint8_t>& data) {
    return writeHostFile(devicePrefix + fileName, devicePrefix + fileName, data);
}

SwitchMountResult SwitchSaveMount::mountTitleOverride(uint64_t titleId) {
    SwitchMountResult out;
    out.modeUsed = SaveAccessMode::TitleOverride;

    if (!isTitleOverrideFor(titleId)) {
        out.status = MountStatus::TitleMismatch;
        out.message =
            "Not running under title override for this game. "
            "Hold R while launching the Pokémon title, then open PKHub.";
        return out;
    }

#if defined(__SWITCH__)
    auto read = readSaveFile("save:/", "main");
    read.modeUsed = SaveAccessMode::TitleOverride;
    if (read.status == MountStatus::Ok) {
        mounted_ = true;
        mountName_ = "save";
        lastMode_ = SaveAccessMode::TitleOverride;
    }
    return read;
#else
    const std::string logical = desktopFixturePath(titleId, "main");
    auto read = readHostFile(fs::resolvePath(logical), logical);
    read.modeUsed = SaveAccessMode::TitleOverride;
    if (read.status == MountStatus::Ok) {
        mounted_ = true;
        lastMode_ = SaveAccessMode::TitleOverride;
        read.message =
            "Desktop title-override fixture (" + logical + "). " + read.message;
    } else {
        read.message =
            "Desktop fixture missing: " + logical +
            " (export PKHUB_TITLE_OVERRIDE=1 and place save bytes there)";
    }
    return read;
#endif
}

SwitchMountResult SwitchSaveMount::mountFsSaveData(uint64_t titleId, const SwitchUserId& user) {
    SwitchMountResult out;
    out.modeUsed = SaveAccessMode::FsSaveData;

#if defined(__SWITCH__)
    AccountUid uid{};
    if (user.isZero()) {
        if (R_FAILED(accountInitialize(AccountServiceType_Application)) ||
            R_FAILED(accountGetPreselectedUser(&uid)) ||
            (uid.uid[0] == 0 && uid.uid[1] == 0)) {
            out.status = MountStatus::UserRequired;
            out.message =
                "No user selected for save mount. Pick a user or use title override (hold R).";
            return out;
        }
    } else {
        uid.uid[0] = user.uid[0];
        uid.uid[1] = user.uid[1];
    }

    FsFileSystem saveFs;
    FsSaveDataAttribute attr{};
    attr.uid = uid;
    attr.application_id = titleId;
    attr.save_data_type = FsSaveDataType_Account;

    const Result rc = fsOpenSaveDataFileSystemBySaveDataAttribute(
        &saveFs, FsSaveDataSpaceId_User, &attr);
    if (R_FAILED(rc)) {
        out.status = MountStatus::PermissionDenied;
        out.message = "fsOpenSaveDataFileSystem failed. Prefer title override (hold R).";
        return out;
    }

    unmount();
    if (fsdevMountDevice(kFsdevName, saveFs) == -1) {
        fsFsClose(&saveFs);
        out.status = MountStatus::IoError;
        out.message = "fsdevMountDevice failed";
        return out;
    }
    mounted_ = true;
    mountName_ = kFsdevName;
    lastMode_ = SaveAccessMode::FsSaveData;

    auto read = readSaveFile(std::string(kFsdevName) + ":/", "main");
    read.modeUsed = SaveAccessMode::FsSaveData;
    if (read.status != MountStatus::Ok) {
        unmount();
    } else {
        read.message = "Mounted via fsOpen_SaveData. " + read.message;
    }
    return read;
#else
    (void)user;
    const std::string logical = desktopFixturePath(titleId, "main");
    auto read = readHostFile(fs::resolvePath(logical), logical);
    read.modeUsed = SaveAccessMode::FsSaveData;
    if (read.status == MountStatus::Ok) {
        mounted_ = true;
        lastMode_ = SaveAccessMode::FsSaveData;
        read.message = "Desktop FsSaveData fixture (" + logical + "). " + read.message;
    } else {
        read.message = "Desktop fixture missing: " + logical;
    }
    return read;
#endif
}

SwitchMountResult SwitchSaveMount::mountAndRead(const SwitchMountRequest& req) {
    unmount();
    if (req.titleId == 0) {
        SwitchMountResult out;
        out.status = MountStatus::NotFound;
        out.message = "Title ID is zero (game stub / unknown title)";
        return out;
    }

    switch (req.mode) {
        case SaveAccessMode::TitleOverride:
            return mountTitleOverride(req.titleId);
        case SaveAccessMode::FsSaveData:
            return mountFsSaveData(req.titleId, req.user);
        case SaveAccessMode::Auto:
        default: {
            auto first = mountTitleOverride(req.titleId);
            if (first.status == MountStatus::Ok) {
                return first;
            }
            auto second = mountFsSaveData(req.titleId, req.user);
            if (second.status == MountStatus::Ok) {
                return second;
            }
            SwitchMountResult out = second;
            out.status = (first.status == MountStatus::TitleMismatch) ? first.status
                                                                      : second.status;
            out.message = "Auto mount failed. [Title override] " + first.message +
                          " | [FsSaveData] " + second.message;
            return out;
        }
    }
}

SwitchMountResult SwitchSaveMount::write(const SwitchMountRequest& req,
                                         SaveAccessMode modeUsed,
                                         const std::string& saveFileName,
                                         const std::vector<uint8_t>& data) {
    const std::string file = saveFileName.empty() ? "main" : saveFileName;

#if defined(__SWITCH__)
    SwitchMountRequest r = req;
    r.mode = modeUsed;
    if (!mounted_ || lastMode_ != modeUsed) {
        auto m = mountAndRead(r);
        if (m.status != MountStatus::Ok) {
            return m;
        }
    }
    const std::string prefix = (modeUsed == SaveAccessMode::TitleOverride)
                                   ? "save:/"
                                   : (std::string(kFsdevName) + ":/");
    auto w = writeSaveFile(prefix, file, data);
    w.modeUsed = modeUsed;
    return w;
#else
    (void)modeUsed;
    const std::string logical = desktopFixturePath(req.titleId, file);
    const auto slash = logical.find_last_of('/');
    if (slash != std::string::npos) {
        fs::createDirectories(logical.substr(0, slash));
    }
    auto w = writeHostFile(fs::resolvePath(logical), logical, data);
    w.modeUsed = modeUsed;
    return w;
#endif
}

}  // namespace pkhub
