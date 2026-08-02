/**
 * PKHub entrypoint — Borealis UI when linked, otherwise headless smoke.
 */

#include <cstdio>
#include <memory>
#include <string>

#include "pkhub/app/AppContext.hpp"
#include "pkhub/app/Version.hpp"
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
#include "pkhub/ui/BrandBanner.hpp"
#include "pkhub/ui/UiList.hpp"
#include "pkhub/ui/activities/FileBrowserActivity.hpp"
#include "pkhub/ui/activities/MainActivity.hpp"
#include "pkhub/ui/views/BoxGridView.hpp"
#include "pkhub/ui/views/PokemonSlotView.hpp"
#endif

namespace {

#if defined(PKHUB_HAS_BOREALIS)

using namespace pkhub;
using pkhub::ui::addBodyLabel;
using pkhub::ui::addClickableDetail;
using pkhub::ui::addTipBlock;
using pkhub::ui::makeAppletFrame;
using pkhub::ui::makeBrandBanner;
using pkhub::ui::makeScrollList;
using pkhub::ui::makeSectionHeader;

static std::string gameStatusDetail(const DetectedSave& d) {
    if (!d.formatSupported) {
        return "Coming soon";
    }
    if (SwitchSaveMount::isTitleOverrideFor(d.titleId)) {
        return "Ready · override active";
    }
    return "Ready · hold R first";
}

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
    list.content->addView(makeBrandBanner("First beta · official Switch saves"));

    addTipBlock(list.content, "Quick tip",
                "Hold R while launching the game, then open PKHub.");

    auto users = listSwitchUsers();
    if (!users.empty()) {
        addClickableDetail(list.content, "Profiles on this Switch",
                           std::to_string(users.size()) + " user(s)", nullptr);
    }

    list.content->addView(makeSectionHeader("Games"));
    for (const auto& d : scanKnownSwitchTitles()) {
        DetectedSave detected = d;
        addClickableDetail(list.content, d.displayName, gameStatusDetail(d),
                           [detected, users](brls::View*) {
                               if (users.size() <= 1) {
                                   SwitchUserId uid =
                                       users.empty() ? SwitchUserId{} : users[0].id;
                                   return openSwitchDetected(detected, uid);
                               }
                               auto picker = makeScrollList();
                               picker.content->addView(
                                   makeBrandBanner("Choose a profile"));
                               picker.content->addView(makeSectionHeader("Profiles"));
                               for (const auto& u : users) {
                                   DetectedSave det = detected;
                                   SwitchUserId uid = u.id;
                                   addClickableDetail(
                                       picker.content,
                                       u.nickname.empty() ? "User" : u.nickname,
                                       "Open save for this profile",
                                       [det, uid](brls::View*) {
                                           return openSwitchDetected(det, uid);
                                       });
                               }
                               addClickableDetail(
                                   picker.content, "Use title override",
                                   "No explicit user id", [detected](brls::View*) {
                                       return openSwitchDetected(detected, {});
                                   });
                               brls::Application::pushActivity(new brls::Activity(
                                   makeAppletFrame("Choose user — " + detected.displayName,
                                                   picker.scroll)));
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
    list.content->addView(makeBrandBanner("RetroArch & raw saves"));
    list.content->addView(makeSectionHeader("Browse"));

    addClickableDetail(
        list.content, "Scan RetroArch saves", "GBA → DS · common SD paths",
        [](brls::View*) {
            auto found = scanRetroArchSaves();
            if (found.empty()) {
                brls::Application::notify("No .sav/.srm/.dsv found in default paths");
                return true;
            }
            auto results = makeScrollList();
            results.content->addView(makeBrandBanner(
                std::to_string(found.size()) + " save(s) found"));
            results.content->addView(makeSectionHeader("Detected"));
            for (const auto& d : found) {
                DetectedSave detected = d;
                addClickableDetail(
                    results.content, d.displayName,
                    d.formatHint + (d.formatSupported ? "" : " · unsupported"),
                    [detected](brls::View*) { return openDetectedRawSave(detected); });
            }
            brls::Application::pushActivity(new brls::Activity(makeAppletFrame(
                "Detected saves", results.scroll)));
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

#if defined(PLATFORM_DESKTOP)
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
#endif

    return list.scroll;
}

static brls::View* buildAboutView() {
    auto list = makeScrollList();
    list.content->addView(makeBrandBanner(std::string(kAppVersion) + " · first public beta"));

    list.content->addView(makeSectionHeader("This beta includes"));
    addBodyLabel(list.content, "Sword / Shield / Scarlet / Violet — SwishCrypto edit + write-back");
    addBodyLabel(list.content, "National Dex sprites + English names (#1–1025, shiny)");
    addBodyLabel(list.content, "GBA Gen 3 + RetroArch save scan");
    addBodyLabel(list.content, "Hub Storage — cross-gen boxes on SD");

    list.content->addView(makeSectionHeader("Coming next"));
    addBodyLabel(list.content, "BDSP / Legends: Arceus full parse (mount works today)");
    addBodyLabel(list.content, "Soft legality — warn first, confirm only high-risk writes");
    return list.scroll;
}

static brls::View* buildHubTab() {
    auto list = makeScrollList();
    list.content->addView(makeBrandBanner("Your multi-gen boxes"));
    list.content->addView(makeSectionHeader("Storage"));
    addClickableDetail(list.content, "Open Hub Storage", "Persistent boxes on SD",
                       [](brls::View*) {
                           auto& ctx = AppContext::instance();
                           brls::Application::pushActivity(new brls::Activity(
                               ui::buildSaveWorkspace(&ctx.hub(), "Hub Storage")));
                           return true;
                       });
    addClickableDetail(list.content, "About this beta",
                       std::string(kAppVersion) + " · what's included",
                       [](brls::View*) {
                           brls::Application::pushActivity(new brls::Activity(
                               makeAppletFrame("About PKHub", buildAboutView())));
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
    root->getAppletFrameItem()->title = std::string(kAppName) + "  " + kAppVersion;
    root->getAppletFrameItem()->setIconFromRes("img/icons/pkhub.png");
    root->addTab("Games", []() { return buildSwitchGamesTab(); });
    root->addTab("Files", []() { return buildEmuTab(); });
    root->addTab("Hub", []() { return buildHubTab(); });

    brls::Application::pushActivity(new brls::Activity(root));

    while (brls::Application::mainLoop()) {
    }
    return 0;
}

#endif

int runHeadlessSmoke() {
    std::printf("%s %s — headless smoke\n", pkhub::kAppName, pkhub::kAppVersion);
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
