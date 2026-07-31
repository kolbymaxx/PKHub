#include "pkhub/platform/SaveAccess.hpp"

#if defined(__SWITCH__)
#include <switch.h>
#include <vector>
#endif

namespace pkhub {

const char* saveAccessModeLabel(SaveAccessMode mode) {
    switch (mode) {
        case SaveAccessMode::TitleOverride: return "Title override";
        case SaveAccessMode::FsSaveData: return "Save data mount";
        case SaveAccessMode::Auto: return "Auto";
        default: return "Unknown";
    }
}

std::vector<SwitchUserInfo> listSwitchUsers() {
    std::vector<SwitchUserInfo> out;
#if defined(__SWITCH__)
    if (R_FAILED(accountInitialize(AccountServiceType_Application))) {
        return out;
    }
    AccountUid uids[8]{};
    s32 total = 0;
    if (R_SUCCEEDED(accountListAllUsers(uids, 8, &total))) {
        for (s32 i = 0; i < total; ++i) {
            SwitchUserInfo info;
            info.id.uid[0] = uids[i].uid[0];
            info.id.uid[1] = uids[i].uid[1];
            AccountProfile profile{};
            AccountProfileBase base{};
            if (R_SUCCEEDED(accountGetProfile(&profile, uids[i])) &&
                R_SUCCEEDED(accountProfileGet(&profile, nullptr, &base))) {
                info.nickname = base.nickname;
                accountProfileClose(&profile);
            } else {
                info.nickname = "User " + std::to_string(i + 1);
            }
            out.push_back(std::move(info));
        }
    }
    accountExit();
#endif
    return out;
}

}  // namespace pkhub
