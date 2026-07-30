#include "pkhub/app/AppContext.hpp"

#include "pkhub/core/fs/Paths.hpp"

namespace pkhub {

AppContext& AppContext::instance() {
    static AppContext ctx;
    return ctx;
}

bool AppContext::initialize() {
    if (ready_) {
        return true;
    }
    fs::ensureAppDirectories();
    dataRoot_ = fs::resolvePath(fs::kAppRoot);

    HubConfig hubCfg;
    hubCfg.rootPath = fs::kHubDir;
    hub_ = std::make_unique<HubStorage>(hubCfg);
    backups_ = std::make_unique<BackupService>(fs::kBackupDir);
    session_ = std::make_unique<SaveSession>(*hub_, *backups_);

    const auto st = hub_->openOrCreate();
    ready_ = (st.result == SaveOpenResult::Ok);
    return ready_;
}

void AppContext::shutdown() {
    if (session_) {
        session_->detachBackend();
    }
    if (hub_ && hub_->isOpen() && hub_->isDirty()) {
        hub_->commit();
    }
    if (hub_) {
        hub_->close();
    }
    ready_ = false;
}

}  // namespace pkhub
