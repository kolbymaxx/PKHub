#include "pkhub/platform/SaveAccess.hpp"

namespace pkhub {

const char* saveAccessModeLabel(SaveAccessMode mode) {
    switch (mode) {
        case SaveAccessMode::TitleOverride: return "Title override";
        case SaveAccessMode::FsSaveData: return "Save data mount";
        case SaveAccessMode::Auto: return "Auto";
        default: return "Unknown";
    }
}

}  // namespace pkhub
