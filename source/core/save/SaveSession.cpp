#include "pkhub/core/save/SaveSession.hpp"

namespace pkhub {

SaveSession::SaveSession(HubStorage& hub, BackupService& backups)
    : hub_(hub), backups_(backups) {}

SaveOpenStatus SaveSession::attachBackend(SaveBackendPtr backend) {
    if (!backend) {
        return {SaveOpenResult::NotFound, "Null backend"};
    }
    auto status = backend->open();
    if (status.result != SaveOpenResult::Ok) {
        return status;
    }
    backend_ = std::move(backend);
    return status;
}

void SaveSession::detachBackend() {
    if (backend_) {
        backend_->close();
        backend_.reset();
    }
}

bool SaveSession::isDirty() const {
    const bool hubDirty = hub_.isDirty();
    const bool beDirty = backend_ && backend_->isDirty();
    return hubDirty || beDirty;
}

SaveOpenStatus SaveSession::saveAll(bool writeBackend, bool writeHub) {
    if (writeBackend) {
        if (!backend_ || !backend_->isOpen()) {
            return {SaveOpenResult::NotFound, "No backend attached"};
        }
        auto bak = backups_.backupBeforeWrite(*backend_, backend_->displayName());
        if (!bak.ok) {
            return {SaveOpenResult::IoError, "Backup failed: " + bak.message};
        }
        auto st = backend_->commit();
        if (st.result != SaveOpenResult::Ok) {
            return st;
        }
    }
    if (writeHub) {
        auto bak = backups_.backupHub(hub_.rootPath(), "hub");
        if (!bak.ok) {
            return {SaveOpenResult::IoError, "Hub backup failed: " + bak.message};
        }
        return hub_.commit();
    }
    return {SaveOpenResult::Ok, {}};
}

}  // namespace pkhub
