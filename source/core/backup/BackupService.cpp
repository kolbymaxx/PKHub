#include "pkhub/core/backup/BackupService.hpp"

namespace pkhub {

BackupService::BackupService(std::string root) : root_(std::move(root)) {}

BackupResult BackupService::backupBeforeWrite(ISaveBackend& backend, const std::string& label) {
    (void)backend;
    // Skeleton: real implementation copies mounted save / raw file into root_/...
    return {true, root_ + "/" + label, "Backup stub OK"};
}

BackupResult BackupService::backupHub(const std::string& hubRoot, const std::string& label) {
    (void)hubRoot;
    return {true, root_ + "/hub/" + label, "Hub backup stub OK"};
}

}  // namespace pkhub
