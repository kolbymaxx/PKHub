#include "pkhub/ui/activities/FileBrowserActivity.hpp"

#include "pkhub/core/fs/FileBrowser.hpp"

#include <memory>

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

brls::View* buildFileBrowser(const std::string& startPath,
                             std::function<void(const std::string& path)> onFileChosen) {
    auto* root = new brls::AppletFrame();
    auto* list = new brls::List();
    auto currentPath = std::make_shared<std::string>(startPath.empty() ? std::string("sdmc:/")
                                                                       : startPath);

    auto rebuild = std::make_shared<std::function<void()>>();
    *rebuild = [list, currentPath, onFileChosen, rebuild, root]() {
        list->clearViews();
        root->setTitle("Browse: " + *currentPath);

        auto* up = new brls::ListItem("..", "Parent folder");
        up->registerClickAction([currentPath, rebuild](brls::View*) {
            const std::string parent = fs::parentPath(*currentPath);
            if (!parent.empty() && parent != *currentPath) {
                *currentPath = parent;
                (*rebuild)();
            }
            return true;
        });
        list->addView(up);

        if (!fs::isDirectory(*currentPath)) {
            list->addView(new brls::ListItem("Path not found", *currentPath));
            return;
        }

        for (const auto& ent : fs::listDirectory(*currentPath)) {
            if (ent.kind == fs::DirEntryKind::Directory) {
                auto* item = new brls::ListItem(ent.name + "/", "Folder");
                const std::string child = ent.path;
                item->registerClickAction([currentPath, child, rebuild](brls::View*) {
                    *currentPath = child;
                    (*rebuild)();
                    return true;
                });
                list->addView(item);
                continue;
            }
            if (ent.kind != fs::DirEntryKind::File) {
                continue;
            }
            const std::string ext = fs::fileExtension(ent.name);
            if (ext != "sav" && ext != "srm" && ext != "dsv") {
                continue;
            }
            auto* item =
                new brls::ListItem(ent.name, ext + " · " + std::to_string(ent.size) + " B");
            const std::string filePath = ent.path;
            item->registerClickAction([filePath, onFileChosen](brls::View*) {
                if (onFileChosen) {
                    onFileChosen(filePath);
                }
                return true;
            });
            list->addView(item);
        }
    };

    (*rebuild)();
    root->setContentView(list);
    return root;
}

#endif

}  // namespace pkhub::ui
