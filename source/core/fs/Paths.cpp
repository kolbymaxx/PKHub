#include "pkhub/core/fs/Paths.hpp"

#include <cstdlib>
#include <cstring>
#include <string>

#if defined(__SWITCH__)
#include <sys/stat.h>
#include <unistd.h>
#else
#include <sys/stat.h>
#endif

namespace pkhub::fs {
namespace {

#if !defined(__SWITCH__)
std::string hostDataRoot() {
    if (const char* env = std::getenv("PKHUB_DATA_ROOT")) {
        if (env[0] != '\0') {
            std::string root = env;
            while (!root.empty() && (root.back() == '/' || root.back() == '\\')) {
                root.pop_back();
            }
            return root;
        }
    }
    return "./pkhub_data";
}

constexpr const char* kSdmcPrefix = "sdmc:/switch/PKHub";
#endif

}  // namespace

std::string resolvePath(const std::string& p) {
#if defined(__SWITCH__)
    return p;
#else
    // Map sdmc:/switch/PKHub/... → $PKHUB_DATA_ROOT/... or ./pkhub_data/...
    if (p.rfind(kSdmcPrefix, 0) == 0) {
        const std::string rest = p.substr(std::strlen(kSdmcPrefix));
        return hostDataRoot() + rest;
    }
    // Map other sdmc:/ paths → data root + /rest
    if (p.rfind("sdmc:", 0) == 0) {
        std::string rest = p.substr(5);  // after "sdmc:"
        if (rest.empty() || rest[0] != '/') {
            rest = "/" + rest;
        }
        return hostDataRoot() + rest;
    }
    // Absolute SD-style paths used by RetroArch defaults
    if (!p.empty() && p[0] == '/') {
        return hostDataRoot() + p;
    }
    return p;
#endif
}

bool createDirectories(const std::string& path) {
#if defined(_WIN32)
    (void)path;
    return false;
#else
    const std::string resolved = resolvePath(path);
    // Minimal mkdir -p style.
    std::string partial;
    for (std::size_t i = 0; i < resolved.size(); ++i) {
        partial.push_back(resolved[i]);
        if (resolved[i] == '/' || i + 1 == resolved.size()) {
            if (partial.size() <= 1) {
                continue;
            }
            // skip scheme-like "sdmc:" if somehow still present
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
    return ::stat(resolvePath(path).c_str(), &st) == 0;
}

bool ensureAppDirectories() {
    return createDirectories(kHubDir) && createDirectories(kBackupDir) &&
           createDirectories(kCacheDir) && createDirectories(kLogDir);
}

}  // namespace pkhub::fs
