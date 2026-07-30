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

static brls::View* buildEmuTab() {
    auto* list = new brls::List();
    auto* openPath = new brls::ListItem("Open raw save path…", "Phase 1: set PKHUB_TEST_SAVE");
    openPath->registerClickAction([](brls::View*) {
        const char* path = std::getenv("PKHUB_TEST_SAVE");
        if (!path || !*path) {
            brls::Application::notify("Set PKHUB_TEST_SAVE to a .sav/.srm file");
            return true;
        }
        DetectedSave d;
        d.path = path;
        d.displayName = path;
        RawSaveBackendFactory factory;
        auto backend = factory.create(d);
        auto& ctx = AppContext::instance();
        auto st = ctx.session().attachBackend(std::move(backend));
        if (st.result != SaveOpenResult::Ok) {
            brls::Application::notify(st.message);
            return true;
        }
        brls::Application::pushActivity(new brls::Activity(
            ui::buildSaveWorkspace(ctx.session().backend(), path)));
        return true;
    });
    list->addView(openPath);
    list->addView(new brls::ListItem("RetroArch scan", "Coming next"));
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
