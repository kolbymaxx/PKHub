#pragma once

#include <memory>
#include <string>

#include "pkhub/core/backup/BackupService.hpp"
#include "pkhub/core/hub/HubStorage.hpp"
#include "pkhub/core/save/SaveSession.hpp"
#include "pkhub/services/TransferService.hpp"

namespace pkhub {

/**
 * Process-wide app services shared by UI and headless paths.
 */
class AppContext {
public:
    static AppContext& instance();

    bool initialize();
    void shutdown();

    HubStorage& hub() { return *hub_; }
    BackupService& backups() { return *backups_; }
    SaveSession& session() { return *session_; }
    TransferService& transfer() { return transfer_; }

    const std::string& dataRoot() const { return dataRoot_; }

private:
    AppContext() = default;

    std::string dataRoot_;
    std::unique_ptr<HubStorage> hub_;
    std::unique_ptr<BackupService> backups_;
    std::unique_ptr<SaveSession> session_;
    TransferService transfer_;
    bool ready_ = false;
};

}  // namespace pkhub
