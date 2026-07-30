#include "pkhub/core/hub/HubStorage.hpp"
#include "pkhub/core/pokemon/Pokemon.hpp"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <string>

namespace {

std::string makeTempHubRoot() {
    namespace fs = std::filesystem;
    const fs::path base = fs::temp_directory_path() / "pkhub_hub_test";
    std::error_code ec;
    fs::create_directories(base, ec);
    const auto stamp = std::to_string(static_cast<long long>(std::time(nullptr)));
    fs::path root = base / ("hub_" + stamp);
    // Avoid collisions if tests re-run in the same second.
    for (int i = 0; i < 1000; ++i) {
        root = base / ("hub_" + stamp + "_" + std::to_string(i));
        if (!fs::exists(root)) {
            fs::create_directories(root, ec);
            return root.string();
        }
    }
    return (base / ("hub_" + stamp)).string();
}

}  // namespace

int main() {
    using namespace pkhub;

    const std::string root = makeTempHubRoot();
    std::printf("hub_persist_test root: %s\n", root.c_str());

    {
        HubConfig cfg;
        cfg.rootPath = root;
        cfg.defaultBoxCount = 2;
        cfg.slotsPerBox = 30;

        HubStorage hub(cfg);
        auto st = hub.openOrCreate();
        assert(st.result == SaveOpenResult::Ok);
        assert(hub.isOpen());
        assert(hub.boxCount() == 2);

        Pokemon mon;
        mon.species = 25;  // Pikachu
        mon.level = 42;
        mon.isShiny = true;
        mon.nickname = "Sparky";
        mon.otName = "Ash";
        mon.nature = 3;
        mon.abilityId = 9;
        mon.moves[0] = 85;
        mon.ivs.hp = 31;
        mon.evs.spe = 252;
        hub.box(0).slot(0) = mon;

        st = hub.commit();
        assert(st.result == SaveOpenResult::Ok);
        assert(!hub.isDirty());
        hub.close();
        assert(!hub.isOpen());
    }

    {
        HubConfig cfg;
        cfg.rootPath = root;
        cfg.defaultBoxCount = 2;
        cfg.slotsPerBox = 30;

        HubStorage hub(cfg);
        auto st = hub.openOrCreate();
        assert(st.result == SaveOpenResult::Ok);
        assert(hub.boxCount() >= 1);

        const Pokemon& restored = hub.box(0).slot(0);
        assert(!restored.empty());
        assert(restored.species == 25);
        assert(restored.level == 42);
        assert(restored.isShiny);
        assert(restored.nickname == "Sparky");
        assert(restored.otName == "Ash");
        assert(restored.nature == 3);
        assert(restored.abilityId == 9);
        assert(restored.moves[0] == 85);
        assert(restored.ivs.hp == 31);
        assert(restored.evs.spe == 252);
    }

    std::puts("hub_persist_test OK");
    return 0;
}
