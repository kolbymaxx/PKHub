/**
 * PKHub — Nintendo Switch Pokémon save editor / Hub manager
 *
 * Phase 0 skeleton: boots Borealis (when available) or a headless stub.
 * Desktop Borealis target is preferred for UI iteration.
 */

#include <cstdio>
#include <memory>
#include <string>

#include "pkhub/backends/SaveDiscovery.hpp"
#include "pkhub/backends/UnsupportedSaveBackend.hpp"
#include "pkhub/core/backup/BackupService.hpp"
#include "pkhub/core/fs/Paths.hpp"
#include "pkhub/core/hub/HubStorage.hpp"
#include "pkhub/core/pokemon/GameId.hpp"
#include "pkhub/core/save/SaveSession.hpp"
#include "pkhub/core/safety/SafetyPolicy.hpp"

#if defined(PKHUB_HAS_BOREALIS)
#include <borealis.hpp>
#endif

namespace {

constexpr const char* kAppName = "PKHub";
constexpr const char* kAppVersion = "0.1.0-dev";

#if defined(PKHUB_HAS_BOREALIS)

int runBorealisUi() {
    if (!brls::Application::init()) {
        brls::Logger::error("Unable to init Borealis");
        return 1;
    }

    brls::Application::createWindow(std::string(kAppName) + " " + kAppVersion);

    auto* root = new brls::TabFrame();
    root->setTitle(kAppName);

    auto* switchTab = new brls::List();
    switchTab->addView(new brls::ListItem("Scarlet / Violet", "Official Switch save"));
    switchTab->addView(new brls::ListItem("Sword / Shield", "Official Switch save"));
    switchTab->addView(new brls::ListItem("Legends: Arceus", "Official Switch save"));
    switchTab->addView(new brls::ListItem("BDSP", "Official Switch save"));
    switchTab->addView(new brls::ListItem(
        "Legends: Z-A", "Format not yet documented — stub only"));

    auto* emuTab = new brls::List();
    emuTab->addView(new brls::ListItem("Scan RetroArch saves", "GBA → DS → 3DS"));
    emuTab->addView(new brls::ListItem("Browse files…", "Manual .sav / .dsv / .srm"));

    auto* hubTab = new brls::List();
    hubTab->addView(new brls::ListItem("Open Hub Storage", "Persistent multi-gen boxes"));
    hubTab->addView(new brls::ListItem("Create box", "Phase 1"));

    root->addTab("Switch Games", switchTab);
    root->addTab("Emulator Saves", emuTab);
    root->addTab("Hub Storage", hubTab);

    brls::Application::pushActivity(new brls::Activity(root));

    while (brls::Application::mainLoop()) {
    }

    return 0;
}

#endif

int runHeadlessSmoke() {
    std::printf("%s %s — headless smoke (Borealis not linked)\n", kAppName, kAppVersion);

    pkhub::fs::ensureAppDirectories();

    pkhub::HubStorage hub;
    pkhub::BackupService backups;
    auto hubStatus = hub.openOrCreate();
    std::printf("Hub: %s\n", hubStatus.message.c_str());

    pkhub::SaveSession session(hub, backups);
    auto detected = pkhub::scanKnownSwitchTitles();
    std::printf("Known Switch titles listed: %zu\n", detected.size());
    for (const auto& d : detected) {
        std::printf("  - %s (0x%016llX)%s\n", d.displayName.c_str(),
                    static_cast<unsigned long long>(d.titleId),
                    d.formatSupported ? "" : " [stub: format not documented]");
    }

    auto za = pkhub::makeLegendsZAStub();
    auto zaStatus = za->open();
    std::printf("Z-A stub: %s\n", zaStatus.message.c_str());

    pkhub::SafetyPolicy safety;
    auto soft = safety.evaluate(pkhub::SafetyAction::InjectLikelyIllegal);
    auto hard = safety.evaluate(pkhub::SafetyAction::RawHexEdit);
    std::printf("Safety: illegal inject=%s raw hex=%s\n",
                soft.gate == pkhub::SafetyGate::SoftWarn ? "soft-warn" : "?",
                hard.gate == pkhub::SafetyGate::RequireConfirm ? "confirm" : "?");

    std::printf("OK — skeleton ready.\n");
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

#if defined(PKHUB_HAS_BOREALIS)
    return runBorealisUi();
#else
    return runHeadlessSmoke();
#endif
}
