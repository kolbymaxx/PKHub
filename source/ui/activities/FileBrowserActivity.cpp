#include "pkhub/ui/activities/FileBrowserActivity.hpp"

#include "pkhub/core/fs/FileBrowser.hpp"
#include "pkhub/ui/UiList.hpp"

#include <cctype>
#include <memory>

namespace pkhub::ui {

#if defined(PKHUB_HAS_BOREALIS)

brls::View* buildFileBrowser(const std::string& startPath,
                             std::function<void(const std::string& path)> onFileChosen) {
    auto list = makeScrollList();
    auto* root = makeAppletFrame("Browse", list.scroll);
    auto currentPath = std::make_shared<std::string>(startPath.empty() ? std::string("sdmc:/")
                                                                       : startPath);
    // Keep content pointer alive for rebuilds.
    auto content = list.content;

    auto rebuild = std::make_shared<std::function<void()>>();
    *rebuild = [content, currentPath, onFileChosen, rebuild, root]() {
        content->clearViews();
        root->setTitle("Browse: " + *currentPath);

        content->addView(makeSectionHeader("Folders & saves"));
        addClickableDetail(content, "..", "Parent folder",
                           [currentPath, rebuild](brls::View*) {
                               const std::string parent = fs::parentPath(*currentPath);
                               if (!parent.empty() && parent != *currentPath) {
                                   *currentPath = parent;
                                   (*rebuild)();
                               }
                               return true;
                           });

        if (!fs::isDirectory(*currentPath)) {
            addClickableDetail(content, "Path not found", *currentPath, nullptr);
            return;
        }

        for (const auto& ent : fs::listDirectory(*currentPath)) {
            if (ent.kind == fs::DirEntryKind::Directory) {
                const std::string child = ent.path;
                addClickableDetail(content, ent.name + "/", "Folder",
                                   [currentPath, child, rebuild](brls::View*) {
                                       *currentPath = child;
                                       (*rebuild)();
                                       return true;
                                   });
                continue;
            }
            if (ent.kind != fs::DirEntryKind::File) {
                continue;
            }
            const std::string ext = fs::fileExtension(ent.name);
            std::string nameLower = ent.name;
            for (char& c : nameLower) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            const bool saveLike = (ext == "sav" || ext == "srm" || ext == "dsv" ||
                                   ext == "bin" || nameLower == "main");
            if (!saveLike) {
                continue;
            }
            const std::string filePath = ent.path;
            const std::string detail =
                (ext.empty() ? nameLower : ext) + " · " + std::to_string(ent.size) + " B";
            addClickableDetail(content, ent.name, detail,
                               [filePath, onFileChosen](brls::View*) {
                                   if (onFileChosen) {
                                       onFileChosen(filePath);
                                   }
                                   return true;
                               });
        }
    };

    (*rebuild)();
    return root;
}

#endif

}  // namespace pkhub::ui
