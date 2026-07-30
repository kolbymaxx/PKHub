#pragma once

#include <string>

#include "pkhub/core/save/ISaveBackend.hpp"

namespace pkhub {

struct BackupResult {
    bool ok = false;
    std::string path;
    std::string message;
};

class BackupService {
public:
    explicit BackupService(std::string root = "sdmc:/switch/PKHub/backups");

    /// Copy current on-disk save / hub snapshot before commit.
    BackupResult backupBeforeWrite(ISaveBackend& backend, const std::string& label);
    BackupResult backupHub(const std::string& hubRoot, const std::string& label);

    /// Copy a single file into a timestamped backup folder.
    BackupResult backupFile(const std::string& path, const std::string& label);

    /// Recursively copy a directory tree into a timestamped backup folder.
    BackupResult backupDirectory(const std::string& path, const std::string& label);

    const std::string& root() const { return root_; }

private:
    std::string makeBackupDir(const std::string& category, const std::string& label) const;

    std::string root_;
};

}  // namespace pkhub
