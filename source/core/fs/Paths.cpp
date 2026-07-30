#include "pkhub/core/fs/Paths.hpp"

#include <string>

#if defined(__SWITCH__)
#include <sys/stat.h>
#include <unistd.h>
#else
#include <sys/stat.h>
#endif

namespace pkhub::fs {

bool createDirectories(const std::string& path) {
#if defined(_WIN32)
    (void)path;
    return false;
#else
    // Minimal mkdir -p style for sdmc paths.
    std::string partial;
    for (std::size_t i = 0; i < path.size(); ++i) {
        partial.push_back(path[i]);
        if (path[i] == '/' || i + 1 == path.size()) {
            if (partial.size() <= 1) {
                continue;
            }
            // skip scheme-like "sdmc:"
            if (partial.find(':') != std::string::npos && partial.back() == ':') {
                continue;
            }
            mkdir(partial.c_str(), 0755);
        }
    }
    return true;
#endif
}

bool fileExists(const std::string& path) {
    struct stat st {};
    return ::stat(path.c_str(), &st) == 0;
}

bool ensureAppDirectories() {
    return createDirectories(kHubDir) && createDirectories(kBackupDir) &&
           createDirectories(kCacheDir) && createDirectories(kLogDir);
}

}  // namespace pkhub::fs
