#pragma once

#include <memory>
#include <string>

#include "pkhub/core/backup/BackupService.hpp"
#include "pkhub/core/hub/HubStorage.hpp"
#include "pkhub/core/save/ISaveBackend.hpp"

namespace pkhub {

/**
 * Owns an open backend (or none) plus shared Hub, dirty tracking,
 * and enforces backup-before-commit policy.
 */
class SaveSession {
public:
    SaveSession(HubStorage& hub, BackupService& backups);

    HubStorage& hub() { return hub_; }
    const HubStorage& hub() const { return hub_; }

    ISaveBackend* backend() { return backend_.get(); }
    const ISaveBackend* backend() const { return backend_.get(); }

    SaveOpenStatus attachBackend(SaveBackendPtr backend);
    void detachBackend();

    bool hasBackend() const { return backend_ != nullptr && backend_->isOpen(); }
    bool isDirty() const;

    /// Backup then commit backend and/or hub as needed.
    SaveOpenStatus saveAll(bool writeBackend, bool writeHub);

private:
    HubStorage& hub_;
    BackupService& backups_;
    SaveBackendPtr backend_;
};

}  // namespace pkhub
