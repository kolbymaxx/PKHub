#include "pkhub/core/backup/BackupService.hpp"

#include "pkhub/backends/RawSaveBackend.hpp"
#include "pkhub/backends/SwitchSaveBackend.hpp"
#include "pkhub/core/fs/Paths.hpp"
#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/platform/SaveAccess.hpp"

#include "json.hpp"

#include <cstdio>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace pkhub {
namespace {

using json = nlohmann::json;

std::string timestampFolderName() {
    const std::time_t now = std::time(nullptr);
    std::tm tm {};
#if defined(_WIN32)
    gmtime_s(&tm, &now);
#else
    gmtime_r(&now, &tm);
#endif
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d-%02d-%02dZ", tm.tm_year + 1900,
                  tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buf;
}

std::string sanitizeLabel(const std::string& label) {
    std::string out;
    out.reserve(label.size());
    for (char c : label) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
        out.push_back(ok ? c : '_');
    }
    if (out.empty()) {
        out = "backup";
    }
    return out;
}

std::string joinPath(const std::string& a, const std::string& b) {
    if (a.empty()) {
        return b;
    }
    if (a.back() == '/') {
        return a + b;
    }
    return a + "/" + b;
}

std::string baseName(const std::string& path) {
    const auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

bool copyFileContents(const std::string& src, const std::string& dst) {
    std::ifstream in(fs::resolvePath(src), std::ios::binary);
    if (!in) {
        return false;
    }
    std::ofstream out(fs::resolvePath(dst), std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out << in.rdbuf();
    return static_cast<bool>(out);
}

bool isDirectory(const std::string& path) {
    struct stat st {};
    if (::stat(fs::resolvePath(path).c_str(), &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

bool copyDirectoryRecursive(const std::string& src, const std::string& dst) {
    if (!fs::createDirectories(dst)) {
        return false;
    }
    DIR* dir = ::opendir(fs::resolvePath(src).c_str());
    if (!dir) {
        return false;
    }
    bool ok = true;
    while (auto* ent = ::readdir(dir)) {
        const std::string name = ent->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        const std::string childSrc = joinPath(src, name);
        const std::string childDst = joinPath(dst, name);
        if (isDirectory(childSrc)) {
            if (!copyDirectoryRecursive(childSrc, childDst)) {
                ok = false;
                break;
            }
        } else {
            if (!copyFileContents(childSrc, childDst)) {
                ok = false;
                break;
            }
        }
    }
    ::closedir(dir);
    return ok;
}

}  // namespace

BackupService::BackupService(std::string root) : root_(std::move(root)) {}

std::string BackupService::makeBackupDir(const std::string& category,
                                         const std::string& label) const {
    const std::string folder =
        joinPath(joinPath(root_, category), sanitizeLabel(label) + "_" + timestampFolderName());
    return folder;
}

BackupResult BackupService::backupFile(const std::string& path, const std::string& label) {
    if (!fs::fileExists(path)) {
        return {false, {}, "Source file not found: " + path};
    }
    const std::string destDir = makeBackupDir("files", label);
    if (!fs::createDirectories(destDir)) {
        return {false, destDir, "Failed to create backup directory"};
    }
    const std::string destPath = joinPath(destDir, baseName(path));
    if (!copyFileContents(path, destPath)) {
        return {false, destPath, "Failed to copy file"};
    }
    return {true, destPath, "File backed up"};
}

BackupResult BackupService::backupDirectory(const std::string& path, const std::string& label) {
    if (!fs::fileExists(path) || !isDirectory(path)) {
        return {false, {}, "Source directory not found: " + path};
    }
    const std::string destDir = makeBackupDir("dirs", label);
    if (!copyDirectoryRecursive(path, destDir)) {
        return {false, destDir, "Failed to copy directory"};
    }
    return {true, destDir, "Directory backed up"};
}

BackupResult BackupService::backupBeforeWrite(ISaveBackend& backend, const std::string& label) {
    if (auto* raw = dynamic_cast<RawSaveBackend*>(&backend)) {
        return backupFile(raw->path(), label.empty() ? "raw_save" : label);
    }

    if (auto* sw = dynamic_cast<SwitchSaveBackend*>(&backend)) {
        const std::string destDir = makeBackupDir("switch", label.empty() ? "switch_save" : label);
        if (!fs::createDirectories(destDir)) {
            return {false, destDir, "Failed to create backup directory"};
        }
        const auto& raw = sw->rawBytes();
        if (!raw.empty()) {
            const std::string mainPath = joinPath(destDir, "main");
            std::ofstream bin(fs::resolvePath(mainPath), std::ios::binary | std::ios::trunc);
            if (!bin) {
                return {false, mainPath, "Failed to write Switch save backup bytes"};
            }
            bin.write(reinterpret_cast<const char*>(raw.data()),
                      static_cast<std::streamsize>(raw.size()));
            if (!bin) {
                return {false, mainPath, "Failed to flush Switch save backup bytes"};
            }
        }
        const std::string markerPath = joinPath(destDir, "backup_marker.json");
        json marker = {
            {"type", "switch_save_backup"},
            {"label", label},
            {"displayName", sw->displayName()},
            {"gameId", static_cast<int>(sw->gameId())},
            {"game", gameDisplayName(sw->gameId())},
            {"bytes", raw.size()},
            {"parseImplemented", sw->parseImplemented()},
            {"accessMode", saveAccessModeLabel(sw->modeUsed())},
            {"timestamp", timestampFolderName()},
        };
        std::ofstream out(fs::resolvePath(markerPath), std::ios::trunc);
        if (!out) {
            return {false, markerPath, "Failed to write switch backup marker"};
        }
        out << marker.dump(2) << '\n';
        if (!out) {
            return {false, markerPath, "Failed to flush switch backup marker"};
        }
        return {true, destDir, raw.empty() ? "Switch save marker written" : "Switch save bytes backed up"};
    }

    // Unknown backend: still create a folder entry so callers can proceed safely.
    const std::string destDir = makeBackupDir("other", label.empty() ? "backend" : label);
    if (!fs::createDirectories(destDir)) {
        return {false, destDir, "Failed to create backup directory"};
    }
    const std::string markerPath = joinPath(destDir, "backup_marker.json");
    json marker = {
        {"type", "unknown_backend_marker"},
        {"label", label},
        {"displayName", backend.displayName()},
        {"timestamp", timestampFolderName()},
    };
    std::ofstream out(fs::resolvePath(markerPath), std::ios::trunc);
    if (!out) {
        return {false, markerPath, "Failed to write backend backup marker"};
    }
    out << marker.dump(2) << '\n';
    return {true, destDir, "Backend marker written"};
}

BackupResult BackupService::backupHub(const std::string& hubRoot, const std::string& label) {
    if (!fs::fileExists(hubRoot)) {
        // Nothing on disk yet — treat as success with empty snapshot folder.
        const std::string destDir = makeBackupDir("hub", label.empty() ? "hub" : label);
        if (!fs::createDirectories(destDir)) {
            return {false, destDir, "Failed to create hub backup directory"};
        }
        return {true, destDir, "Hub path missing; empty backup folder created"};
    }
    if (isDirectory(hubRoot)) {
        const std::string destDir = makeBackupDir("hub", label.empty() ? "hub" : label);
        if (!copyDirectoryRecursive(hubRoot, destDir)) {
            return {false, destDir, "Failed to copy hub directory"};
        }
        return {true, destDir, "Hub backed up"};
    }
    return backupFile(hubRoot, label.empty() ? "hub" : label);
}

}  // namespace pkhub
