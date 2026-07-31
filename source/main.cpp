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
#include "pkhub/platform/SwitchSaveMount.hpp"
#include "pkhub/ui/UiBootstrap.hpp"

#include <cstdlib>

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
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

static brls::View* buildSwitchGamesTab() {
    auto* list = new brls::List();
    list->addView(new brls::ListItem(
        "Title override tip",
        "Hold R while launching the game, then open PKHub for reliable save access"));
    for (const auto& d : scanKnownSwitchTitles()) {
        auto* item = new brls::ListItem(
            d.displayName,
            d.formatSupported
                ? (std::string("Official save · ") +
                   (SwitchSaveMount::isTitleOverrideFor(d.titleId) ? "override active"
                                                                   : "auto mount"))
                : "Format not yet documented");
        DetectedSave detected = d;
        item->registerClickAction([detected](brls::View*) {
            auto& ctx = AppContext::instance();
            SwitchSaveBackendFactory factory;
            auto backend = factory.create(detected);
            if (!backend) {
                brls::Application::notify("Unable to create backend");
                return true;
            }
            auto st = ctx.session().attachBackend(std::move(backend));
            if (st.result != SaveOpenResult::Ok) {
                brls::Application::notify(st.message);
                return true;
            }
            brls::Application::pushActivity(new brls::Activity(
                ui::buildSaveWorkspace(ctx.session().backend(), detected.displayName)));
            return true;
        });
        list->addView(item);
    }
    return list;
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
    auto* list = new brls::List();

    auto* scan = new brls::ListItem("Scan RetroArch saves", "GBA → DS · common SD paths");
    scan->registerClickAction([](brls::View*) {
        auto found = scanRetroArchSaves();
        if (found.empty()) {
            brls::Application::notify("No .sav/.srm/.dsv found in default paths");
            return true;
        }
        auto* results = new brls::List();
        for (const auto& d : found) {
            auto* item = new brls::ListItem(
                d.displayName,
                d.formatHint + (d.formatSupported ? "" : " · unsupported") + " · " + d.path);
            DetectedSave detected = d;
            item->registerClickAction([detected](brls::View*) {
                return openDetectedRawSave(detected);
            });
            results->addView(item);
        }
        auto* frame = new brls::AppletFrame();
        frame->setTitle("Detected saves (" + std::to_string(found.size()) + ")");
        frame->setContentView(results);
        brls::Application::pushActivity(new brls::Activity(frame));
        return true;
    });
    list->addView(scan);

    auto* browse = new brls::ListItem("Browse files…", "Manual .sav / .dsv / .srm");
    browse->registerClickAction([](brls::View*) {
        brls::Application::pushActivity(new brls::Activity(ui::buildFileBrowser(
            "sdmc:/retroarch/saves", [](const std::string& path) {
                auto det = detectRawSaveFile(path);
                if (!det) {
                    brls::Application::notify("Not a recognized save file");
                    return;
                }
                openDetectedRawSave(*det);
            })));
        return true;
    });
    list->addView(browse);

    auto* envOpen = new brls::ListItem("Open PKHUB_TEST_SAVE", "Desktop / debug helper");
    envOpen->registerClickAction([](brls::View*) {
        const char* path = std::getenv("PKHUB_TEST_SAVE");
        if (!path || !*path) {
            brls::Application::notify("Set PKHUB_TEST_SAVE to a .sav/.srm file");
            return true;
        }
        auto det = detectRawSaveFile(path);
        if (!det) {
            brls::Application::notify("Invalid or missing test save");
            return true;
        }
        return openDetectedRawSave(*det);
    });
    list->addView(envOpen);

    return list;
}

static brls::View* buildHubTab() {
    auto* list = new brls::List();
    auto* open = new brls::ListItem("Open Hub Storage", "Persistent multi-gen boxes");
    open->registerClickAction([](brls::View*) {
        auto& ctx = AppContext::instance();
        brls::Application::pushActivity(new brls::Activity(
            ui::buildSaveWorkspace(&ctx.hub(), "Hub Storage")));
        return true;
    });
    list->addView(open);
    return list;
}

int runBorealisUi() {
    if (!brls::Application::init()) {
        brls::Logger::error("Unable to init Borealis");
        return 1;
    }

    pkhub::ui::registerCustomViews();
    brls::Application::createWindow(std::string(kAppName) + " " + kAppVersion);

    auto* root = new brls::TabFrame();
    root->setTitle(kAppName);
    root->addTab("Switch Games", buildSwitchGamesTab());
    root->addTab("Emulator Saves", buildEmuTab());
    root->addTab("Hub Storage", buildHubTab());

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
