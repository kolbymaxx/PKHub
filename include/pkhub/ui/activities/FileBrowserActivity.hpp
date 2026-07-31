#pragma once

#include <functional>
#include <string>

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
#endif

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

/// Directory browser for emulator saves. Calls onFileChosen with the logical path.
brls::View* buildFileBrowser(const std::string& startPath,
                             std::function<void(const std::string& path)> onFileChosen);

#endif

}  // namespace pkhub::ui
