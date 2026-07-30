#pragma once

#include <string>

namespace pkhub::fs {

constexpr const char* kAppRoot = "sdmc:/switch/PKHub";
constexpr const char* kHubDir = "sdmc:/switch/PKHub/hub";
constexpr const char* kBackupDir = "sdmc:/switch/PKHub/backups";
constexpr const char* kCacheDir = "sdmc:/switch/PKHub/cache";
constexpr const char* kLogDir = "sdmc:/switch/PKHub/logs";
constexpr const char* kConfigPath = "sdmc:/switch/PKHub/config.json";

/**
 * Resolve a logical SD path for host I/O.
 * On Switch: returned unchanged.
 * On desktop:
 *  - `sdmc:/switch/PKHub/...` → `$PKHUB_DATA_ROOT/...` or `./pkhub_data/...`
 *  - other `sdmc:/...` → data root + absolute path after `sdmc:`
 *  - absolute `/...` → data root + path (so `/retroarch/saves` works in tests)
 */
std::string resolvePath(const std::string& p);

bool ensureAppDirectories();
bool fileExists(const std::string& path);
bool createDirectories(const std::string& path);

}  // namespace pkhub::fs
