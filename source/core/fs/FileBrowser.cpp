#include "pkhub/core/fs/FileBrowser.hpp"

#include "pkhub/core/fs/Paths.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <queue>
#include <sys/stat.h>

#if !defined(_WIN32)
#include <dirent.h>
#endif

namespace pkhub::fs {
namespace {

std::string toLower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

}  // namespace

std::string joinPath(const std::string& parent, const std::string& child) {
    if (parent.empty()) {
        return child;
    }
    if (child.empty()) {
        return parent;
    }
    if (parent.back() == '/') {
        return parent + child;
    }
    return parent + "/" + child;
}

std::string parentPath(const std::string& path) {
    if (path.empty()) {
        return {};
    }
    std::string p = path;
    while (p.size() > 1 && p.back() == '/') {
        p.pop_back();
    }
    const auto pos = p.find_last_of('/');
    if (pos == std::string::npos) {
        return {};
    }
    if (pos == 0) {
        return "/";
    }
    // Keep sdmc: roots intact (sdmc:/foo → sdmc:/)
    if (p.rfind("sdmc:", 0) == 0 && pos <= 5) {
        return "sdmc:/";
    }
    return p.substr(0, pos);
}

std::string fileExtension(const std::string& path) {
    const auto slash = path.find_last_of('/');
    const auto base = (slash == std::string::npos) ? path : path.substr(slash + 1);
    const auto dot = base.find_last_of('.');
    if (dot == std::string::npos || dot + 1 >= base.size()) {
        return {};
    }
    return toLower(base.substr(dot + 1));
}

bool isDirectory(const std::string& path) {
    struct stat st {};
    if (::stat(resolvePath(path).c_str(), &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

bool isRegularFile(const std::string& path) {
    struct stat st {};
    if (::stat(resolvePath(path).c_str(), &st) != 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

std::vector<DirEntry> listDirectory(const std::string& path) {
    std::vector<DirEntry> out;
#if defined(_WIN32)
    (void)path;
    return out;
#else
    const std::string resolved = resolvePath(path);
    DIR* dir = ::opendir(resolved.c_str());
    if (!dir) {
        return out;
    }
    while (dirent* ent = ::readdir(dir)) {
        const char* name = ent->d_name;
        if (!name || std::strcmp(name, ".") == 0 || std::strcmp(name, "..") == 0) {
            continue;
        }
        DirEntry e;
        e.name = name;
        e.path = joinPath(path, name);
        struct stat st {};
        if (::stat(resolvePath(e.path).c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                e.kind = DirEntryKind::Directory;
            } else if (S_ISREG(st.st_mode)) {
                e.kind = DirEntryKind::File;
                e.size = static_cast<uint64_t>(st.st_size);
            } else {
                e.kind = DirEntryKind::Other;
            }
        }
        out.push_back(std::move(e));
    }
    ::closedir(dir);
    std::sort(out.begin(), out.end(), [](const DirEntry& a, const DirEntry& b) {
        if (a.kind != b.kind) {
            return a.kind == DirEntryKind::Directory && b.kind != DirEntryKind::Directory;
        }
        return a.name < b.name;
    });
    return out;
#endif
}

std::vector<std::string> findFilesWithExtensions(const std::string& root,
                                                 const std::vector<std::string>& extensions,
                                                 int maxDepth,
                                                 std::size_t maxFiles) {
    std::vector<std::string> found;
    if (!isDirectory(root) || maxFiles == 0) {
        return found;
    }

    std::vector<std::string> exts;
    exts.reserve(extensions.size());
    for (const auto& e : extensions) {
        exts.push_back(toLower(e));
    }

    struct Node {
        std::string path;
        int depth;
    };
    std::queue<Node> q;
    q.push({root, 0});

    while (!q.empty() && found.size() < maxFiles) {
        const Node cur = q.front();
        q.pop();
        for (const auto& ent : listDirectory(cur.path)) {
            if (found.size() >= maxFiles) {
                break;
            }
            if (ent.kind == DirEntryKind::Directory) {
                if (cur.depth < maxDepth) {
                    q.push({ent.path, cur.depth + 1});
                }
                continue;
            }
            if (ent.kind != DirEntryKind::File) {
                continue;
            }
            const std::string ext = fileExtension(ent.name);
            if (ext.empty()) {
                continue;
            }
            if (std::find(exts.begin(), exts.end(), ext) != exts.end()) {
                found.push_back(ent.path);
            }
        }
    }
    std::sort(found.begin(), found.end());
    return found;
}

}  // namespace pkhub::fs
