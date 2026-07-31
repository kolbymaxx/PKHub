#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pkhub::fs {

enum class DirEntryKind : uint8_t {
    File = 0,
    Directory,
    Other,
};

struct DirEntry {
    std::string name;
    std::string path;  // full logical path
    DirEntryKind kind = DirEntryKind::Other;
    uint64_t size = 0;
};

/// List immediate children of a directory (resolved via resolvePath).
/// Returns empty vector if path missing or not a directory.
std::vector<DirEntry> listDirectory(const std::string& path);

/// Join parent + child with a single '/'.
std::string joinPath(const std::string& parent, const std::string& child);

/// Parent directory of path, or empty if none.
std::string parentPath(const std::string& path);

/// Lowercase file extension without dot, or empty.
std::string fileExtension(const std::string& path);

bool isDirectory(const std::string& path);
bool isRegularFile(const std::string& path);

/// Recursively collect files under root matching extensions (lowercase, no dot).
/// maxDepth: 0 = only root, 1 = one level of subdirs, etc.
std::vector<std::string> findFilesWithExtensions(const std::string& root,
                                                 const std::vector<std::string>& extensions,
                                                 int maxDepth = 4,
                                                 std::size_t maxFiles = 500);

}  // namespace pkhub::fs
