#pragma once

#include <string>

namespace pkhub::fs {

constexpr const char* kAppRoot = "sdmc:/switch/PKHub";
constexpr const char* kHubDir = "sdmc:/switch/PKHub/hub";
constexpr const char* kBackupDir = "sdmc:/switch/PKHub/backups";
constexpr const char* kCacheDir = "sdmc:/switch/PKHub/cache";
constexpr const char* kLogDir = "sdmc:/switch/PKHub/logs";
constexpr const char* kConfigPath = "sdmc:/switch/PKHub/config.json";

/// On desktop (!__SWITCH__), remap `sdmc:/switch/PKHub/...` under PKHUB_DATA_ROOT or `./pkhub_data/...`.
/// On Switch, returns `p` unchanged.
std::string resolvePath(const std::string& p);

bool ensureAppDirectories();
bool fileExists(const std::string& path);
bool createDirectories(const std::string& path);

}  // namespace pkhub::fs
