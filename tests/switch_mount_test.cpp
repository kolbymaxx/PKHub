#include "pkhub/backends/SwitchSaveBackend.hpp"
#include "pkhub/backends/switch/PokeCrypto8.hpp"
#include "pkhub/backends/switch/SwishCrypto.hpp"
#include "pkhub/backends/switch/SwitchSaveParser.hpp"
#include "pkhub/platform/SwitchSaveMount.hpp"
#include "pkhub/platform/TitleIds.hpp"
#include "pkhub/core/fs/Paths.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {

constexpr uint32_t kBox = 0x0d66012c;
constexpr uint32_t kPartySv = 0x3AA1A9AD;

std::vector<uint8_t> buildMinimalSvSave() {
    using pkhub::ScBlock;
    using pkhub::ScType;
    using pkhub::SwishCrypto;

    constexpr std::size_t boxes = 32;
    constexpr std::size_t slots = 30;
    constexpr std::size_t kSlot = pkhub::poke_crypto8::kPartySize;
    ScBlock box;
    box.key = kBox;
    box.type = ScType::Object;
    box.data.assign(boxes * slots * kSlot, 0);

    ScBlock party;
    party.key = kPartySv;
    party.type = ScType::Object;
    party.data.assign(6 * kSlot + 1, 0);

    std::vector<ScBlock> blocks{box, party};
    std::vector<uint8_t> enc;
    std::string err;
    assert(SwishCrypto::encrypt(blocks, enc, &err));
    return enc;
}

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

    // Invalid buffer is rejected for Swish titles
    {
        std::vector<uint8_t> fake(4096, 0x11);
        auto parsed = parseSwitchSave(GameId::Violet, fake);
        assert(!parsed.ok);
    }

    // BDSP still scaffolds empty boxes without Swish hash
    {
        std::vector<uint8_t> fake(4096, 0x11);
        auto parsed = parseSwitchSave(GameId::BrilliantDiamond, fake);
        assert(parsed.ok);
        assert(!parsed.parseImplemented);
    }

    const auto saveBytes = buildMinimalSvSave();

    {
        unsetenv("PKHUB_TITLE_OVERRIDE");
        writeFixture(Scarlet, saveBytes);

        SwitchSaveBackend backend(GameId::Scarlet, Scarlet, {}, SaveAccessMode::FsSaveData);
        auto st = backend.open();
        assert(st.result == SaveOpenResult::Ok);
        assert(backend.boxCount() == 32);
        assert(backend.modeUsed() == SaveAccessMode::FsSaveData);
        assert(backend.parseImplemented());

        st = backend.commit();
        assert(st.result == SaveOpenResult::Ok);
        backend.close();
    }

    {
        setenv("PKHUB_TITLE_OVERRIDE", "1", 1);
        writeFixture(Violet, saveBytes);
        SwitchSaveBackend backend(GameId::Violet, Violet, {}, SaveAccessMode::TitleOverride);
        auto st = backend.open();
        assert(st.result == SaveOpenResult::Ok);
        assert(backend.modeUsed() == SaveAccessMode::TitleOverride);
        backend.close();
        unsetenv("PKHUB_TITLE_OVERRIDE");
    }

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
