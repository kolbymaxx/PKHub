/**
 * PKHub entrypoint — Borealis UI when linked, otherwise headless smoke.
 */

#include <cstdio>
#include <memory>
#include <string>

#include "pkhub/app/AppContext.hpp"
#include "pkhub/backends/SaveDiscovery.hpp"
#include "pkhub/backends/RawSaveBackend.hpp"
#include "pkhub/backends/SwitchSaveBackend.hpp"
#include "pkhub/backends/UnsupportedSaveBackend.hpp"
#include "pkhub/core/safety/SafetyPolicy.hpp"
#include "pkhub/platform/SaveAccess.hpp"
#include "pkhub/platform/SwitchSaveMount.hpp"
#include "pkhub/ui/UiBootstrap.hpp"

#include <cstdlib>

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
#include <borealis/views/cells/cell_detail.hpp>
#include "pkhub/ui/UiList.hpp"
#include "pkhub/ui/activities/FileBrowserActivity.hpp"
#include "pkhub/ui/activities/MainActivity.hpp"
#include "pkhub/ui/views/BoxGridView.hpp"
#include "pkhub/ui/views/PokemonSlotView.hpp"
#endif

namespace {

constexpr const char* kAppName = "PKHub";
constexpr const char* kAppVersion = "0.1.0-dev";

#if defined(PKHUB_HAS_BOREALIS)

using namespace pkhub;
using pkhub::ui::addClickableDetail;
using pkhub::ui::makeAppletFrame;
using pkhub::ui::makeScrollList;
using pkhub::ui::makeSectionHeader;

static bool openSwitchDetected(const DetectedSave& detected, const SwitchUserId& user) {
    auto& ctx = AppContext::instance();
    if (!detected.formatSupported) {
        brls::Application::notify(detected.unsupportedReason.empty()
                                      ? "Format not yet documented"
                                      : detected.unsupportedReason);
        return true;
    }
    auto backend = std::make_unique<SwitchSaveBackend>(
        detected.game, detected.titleId, user, SaveAccessMode::Auto);
    auto st = ctx.session().attachBackend(std::move(backend));
    if (st.result != SaveOpenResult::Ok) {
        brls::Application::notify(st.message);
        return true;
    }
    brls::Application::pushActivity(new brls::Activity(
        ui::buildSaveWorkspace(ctx.session().backend(), detected.displayName)));
    return true;
}

static brls::View* buildSwitchGamesTab() {
    auto list = makeScrollList();
    list.content->addView(makeSectionHeader("Tips"));
    addClickableDetail(list.content, "Title override",
                       "Hold R while launching the game, then open PKHub",
                       [](brls::View*) {
                           brls::Application::notify(
                               "Hold R on game launch for reliable save access");
                           return true;
                       });

    auto users = listSwitchUsers();
    if (!users.empty()) {
        addClickableDetail(list.content, "Users found", std::to_string(users.size()),
                           nullptr);
    }

    list.content->addView(makeSectionHeader("Games"));
    for (const auto& d : scanKnownSwitchTitles()) {
        const std::string detail =
            d.formatSupported
                ? (std::string("Official save · ") +
                   (SwitchSaveMount::isTitleOverrideFor(d.titleId) ? "override active"
                                                                   : "auto mount"))
                : "Format not yet documented";
        DetectedSave detected = d;
        addClickableDetail(list.content, d.displayName, detail,
                           [detected, users](brls::View*) {
                               if (users.size() <= 1) {
                                   SwitchUserId uid =
                                       users.empty() ? SwitchUserId{} : users[0].id;
                                   return openSwitchDetected(detected, uid);
                               }
                               auto picker = makeScrollList();
                               picker.content->addView(makeSectionHeader("Profile"));
                               for (const auto& u : users) {
                                   DetectedSave det = detected;
                                   SwitchUserId uid = u.id;
                                   addClickableDetail(
                                       picker.content,
                                       u.nickname.empty() ? "User" : u.nickname,
                                       "FsSaveData mount with this profile",
                                       [det, uid](brls::View*) {
                                           return openSwitchDetected(det, uid);
                                       });
                               }
                               addClickableDetail(
                                   picker.content, "Use title override / preselected",
                                   "No explicit user id", [detected](brls::View*) {
                                       return openSwitchDetected(detected, {});
                                   });
                               auto* frame = makeAppletFrame("Choose user — " + detected.displayName, picker.scroll);
                               brls::Application::pushActivity(new brls::Activity(frame));
                               return true;
                           });
    }
    return list.scroll;
}

static bool openDetectedRawSave(const DetectedSave& detected) {
    if (!detected.formatSupported) {
        brls::Application::notify(detected.unsupportedReason.empty()
                                      ? "Format not supported yet"
                                      : detected.unsupportedReason);
        return true;
    }
    RawSaveBackendFactory factory;
    auto backend = factory.create(detected);
    auto& ctx = AppContext::instance();
    auto st = ctx.session().attachBackend(std::move(backend));
    if (st.result != SaveOpenResult::Ok) {
        brls::Application::notify(st.message);
        return true;
    }
    brls::Application::pushActivity(new brls::Activity(
        ui::buildSaveWorkspace(ctx.session().backend(), detected.displayName)));
    return true;
}

static brls::View* buildEmuTab() {
    auto list = makeScrollList();
    list.content->addView(makeSectionHeader("Emulator saves"));

    addClickableDetail(
        list.content, "Scan RetroArch saves", "GBA → DS · common SD paths",
        [](brls::View*) {
            auto found = scanRetroArchSaves();
            if (found.empty()) {
                brls::Application::notify("No .sav/.srm/.dsv found in default paths");
                return true;
            }
            auto results = makeScrollList();
            results.content->addView(makeSectionHeader("Detected"));
            for (const auto& d : found) {
                DetectedSave detected = d;
                addClickableDetail(
                    results.content, d.displayName,
                    d.formatHint + (d.formatSupported ? "" : " · unsupported") + " · " +
                        d.path,
                    [detected](brls::View*) { return openDetectedRawSave(detected); });
            }
            auto* frame = makeAppletFrame("Detected saves (" + std::to_string(found.size()) + ")", results.scroll);
            brls::Application::pushActivity(new brls::Activity(frame));
            return true;
        });

    addClickableDetail(list.content, "Browse files…", "Manual .sav / .dsv / .srm",
                       [](brls::View*) {
                           brls::Application::pushActivity(new brls::Activity(
                               ui::buildFileBrowser("sdmc:/retroarch/saves",
                                                    [](const std::string& path) {
                                                        auto det = detectRawSaveFile(path);
                                                        if (!det) {
                                                            brls::Application::notify(
                                                                "Not a recognized save file");
                                                            return;
                                                        }
                                                        openDetectedRawSave(*det);
                                                    })));
                           return true;
                       });

    addClickableDetail(list.content, "Open PKHUB_TEST_SAVE", "Desktop / debug helper",
                       [](brls::View*) {
                           const char* path = std::getenv("PKHUB_TEST_SAVE");
                           if (!path || !*path) {
                               brls::Application::notify(
                                   "Set PKHUB_TEST_SAVE to a .sav/.srm file");
                               return true;
                           }
                           auto det = detectRawSaveFile(path);
                           if (!det) {
                               brls::Application::notify("Invalid or missing test save");
                               return true;
                           }
                           return openDetectedRawSave(*det);
                       });

    return list.scroll;
}

static brls::View* buildHubTab() {
    auto list = makeScrollList();
    list.content->addView(makeSectionHeader("Hub"));
    addClickableDetail(list.content, "Open Hub Storage", "Persistent multi-gen boxes",
                       [](brls::View*) {
                           auto& ctx = AppContext::instance();
                           brls::Application::pushActivity(new brls::Activity(
                               ui::buildSaveWorkspace(&ctx.hub(), "Hub Storage")));
                           return true;
                       });
    addClickableDetail(list.content, "About PKHub",
                       std::string(kAppVersion) + " · soft legality · clean-room formats",
                       [](brls::View*) {
                           brls::Application::notify("PKHub — multi-gen save editor");
                           return true;
                       });
    return list.scroll;
}

int runBorealisUi() {
    if (!brls::Application::init()) {
        brls::Logger::error("Unable to init Borealis");
        return 1;
    }

    pkhub::ui::registerCustomViews();
    brls::Application::createWindow(std::string(kAppName) + " " + kAppVersion);

    auto* root = new brls::TabFrame();
    root->getAppletFrameItem()->title = kAppName;
    root->addTab("Switch Games", []() { return buildSwitchGamesTab(); });
    root->addTab("Emulator Saves", []() { return buildEmuTab(); });
    root->addTab("Hub Storage", []() { return buildHubTab(); });

    brls::Application::pushActivity(new brls::Activity(root));

    while (brls::Application::mainLoop()) {
    }
    return 0;
}

#endif

int runHeadlessSmoke() {
    std::printf("%s %s — headless smoke\n", kAppName, kAppVersion);
    auto& ctx = pkhub::AppContext::instance();
    if (!ctx.initialize()) {
        std::printf("Failed to init AppContext\n");
        return 1;
    }

    auto detected = pkhub::scanKnownSwitchTitles();
    std::printf("Switch titles: %zu\n", detected.size());
    for (const auto& d : detected) {
        std::printf("  - %s%s\n", d.displayName.c_str(),
                    d.formatSupported ? "" : " [stub]");
    }

    auto za = pkhub::makeLegendsZAStub();
    std::printf("Z-A: %s\n", za->open().message.c_str());

    auto emu = pkhub::scanRetroArchSaves();
    std::printf("Emulator saves scanned: %zu\n", emu.size());

    pkhub::SafetyPolicy safety;
    auto soft = safety.evaluate(pkhub::SafetyAction::InjectLikelyIllegal);
    auto hard = safety.evaluate(pkhub::SafetyAction::RawHexEdit);
    std::printf("Safety soft=%d confirm=%d\n",
                soft.gate == pkhub::SafetyGate::SoftWarn,
                hard.gate == pkhub::SafetyGate::RequireConfirm);

    std::printf("Hub boxes: %zu\n", ctx.hub().boxCount());
    ctx.shutdown();
    std::printf("OK\n");
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    auto& ctx = pkhub::AppContext::instance();
    ctx.initialize();

#if defined(PKHUB_HAS_BOREALIS)
    const int rc = runBorealisUi();
#else
    const int rc = runHeadlessSmoke();
#endif
    ctx.shutdown();
    return rc;
}
