#include "pkhub/backends/SwitchSaveBackend.hpp"
#include "pkhub/backends/switch/SwitchSaveParser.hpp"
#include "pkhub/platform/SwitchSaveMount.hpp"
#include "pkhub/platform/TitleIds.hpp"
#include "pkhub/core/fs/Paths.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {

void writeFixture(uint64_t titleId, const std::vector<uint8_t>& bytes) {
    const std::string logical = pkhub::SwitchSaveMount::desktopFixturePath(titleId, "main");
    const auto slash = logical.find_last_of('/');
    assert(slash != std::string::npos);
    assert(pkhub::fs::createDirectories(logical.substr(0, slash)));
    const std::string host = pkhub::fs::resolvePath(logical);
    std::ofstream out(host, std::ios::binary | std::ios::trunc);
    assert(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
}

}  // namespace

int main() {
    using pkhub::GameId;
    using pkhub::SaveAccessMode;
    using pkhub::SaveOpenResult;
    using pkhub::SwitchSaveBackend;
    using pkhub::SwitchSaveMount;
    using pkhub::parseSwitchSave;
    using pkhub::switchBoxCountFor;
    using pkhub::title_ids::Scarlet;
    using pkhub::title_ids::Violet;

    assert(switchBoxCountFor(GameId::Scarlet) == 32);
    assert(switchBoxCountFor(GameId::Sword) == 32);
    assert(switchBoxCountFor(GameId::BrilliantDiamond) == 40);

    // Parser: empty boxes, not yet decrypting Pokémon
    {
        std::vector<uint8_t> fake(4096, 0x11);
        auto parsed = parseSwitchSave(GameId::Violet, fake);
        assert(parsed.ok);
        assert(!parsed.parseImplemented);
        assert(parsed.boxes.size() == 32);
        assert(parsed.party.occupiedCount() == 0);
    }

    // Desktop fixture mount via FsSaveData path (no override env)
    {
        unsetenv("PKHUB_TITLE_OVERRIDE");
        std::vector<uint8_t> bytes(8192, 0x22);
        writeFixture(Scarlet, bytes);

        SwitchSaveBackend backend(GameId::Scarlet, Scarlet, {}, SaveAccessMode::FsSaveData);
        auto st = backend.open();
        assert(st.result == SaveOpenResult::Ok);
        assert(backend.boxCount() == 32);
        assert(backend.modeUsed() == SaveAccessMode::FsSaveData);
        assert(!backend.parseImplemented());

        // Empty commit should rewrite original bytes
        st = backend.commit();
        assert(st.result == SaveOpenResult::Ok);

        // Occupied Pokémon must refuse serialize until parser exists
        backend.box(0).slot(0).species = 25;
        backend.box(0).slot(0).level = 5;
        st = backend.commit();
        assert(st.result == SaveOpenResult::IoError);
        backend.close();
    }

    // Title override path
    {
        setenv("PKHUB_TITLE_OVERRIDE", "1", 1);
        std::vector<uint8_t> bytes(2048, 0x33);
        writeFixture(Violet, bytes);
        SwitchSaveBackend backend(GameId::Violet, Violet, {}, SaveAccessMode::TitleOverride);
        auto st = backend.open();
        assert(st.result == SaveOpenResult::Ok);
        assert(backend.modeUsed() == SaveAccessMode::TitleOverride);
        backend.close();
        unsetenv("PKHUB_TITLE_OVERRIDE");
    }

    // Auto: without override, falls back to fixture FsSaveData
    {
        unsetenv("PKHUB_TITLE_OVERRIDE");
        SwitchSaveBackend backend(GameId::Scarlet, Scarlet, {}, SaveAccessMode::Auto);
        auto st = backend.open();
        assert(st.result == SaveOpenResult::Ok);
        backend.close();
    }

    assert(SwitchSaveMount::desktopFixturePath(Scarlet).find("switch_saves") !=
           std::string::npos);

    std::puts("switch_mount_test OK");
    return 0;
}
