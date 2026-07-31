#include "pkhub/backends/switch/Pk8Codec.hpp"
#include "pkhub/backends/switch/PokeCrypto8.hpp"
#include "pkhub/backends/switch/SwishCrypto.hpp"
#include "pkhub/backends/switch/SwitchSaveParser.hpp"
#include "pkhub/core/pokemon/GameId.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr uint32_t kBox = 0x0d66012c;
constexpr uint32_t kPartySv = 0x3AA1A9AD;

using pkhub::ScBlock;
using pkhub::ScType;
using pkhub::SwishCrypto;

std::vector<uint8_t> makeBlankPk() {
    return std::vector<uint8_t>(pkhub::poke_crypto8::kPartySize, 0);
}

std::vector<uint8_t> makePikachuPk() {
    std::vector<uint8_t> d(pkhub::poke_crypto8::kPartySize, 0);
    // Encryption constant / PID
    d[0] = 0x11;
    d[1] = 0x22;
    d[2] = 0x33;
    d[3] = 0x44;
    // species internal 25
    d[0x08] = 25;
    d[0x09] = 0;
    // TID/SID
    d[0x0C] = 0x34;
    d[0x0D] = 0x12;
    d[0x0E] = 0x78;
    d[0x0F] = 0x56;
    // PID
    d[0x1C] = 0x11;
    d[0x1D] = 0x22;
    d[0x1E] = 0x33;
    d[0x1F] = 0x44;
    d[0x20] = 0;  // nature
    d[0x72] = 84;
    d[0x73] = 0;  // ThunderShock
    d[0x148] = 25;  // level
    pkhub::poke_crypto8::refreshChecksum(d.data());
    pkhub::poke_crypto8::encrypt(d.data(), d.size());
    return d;
}

std::vector<uint8_t> buildSvSave(const std::vector<uint8_t>& slot0) {
    constexpr std::size_t boxes = 32;
    constexpr std::size_t slots = 30;
    constexpr std::size_t kSlot = pkhub::poke_crypto8::kPartySize;

    ScBlock box;
    box.key = kBox;
    box.type = ScType::Object;
    box.data.assign(boxes * slots * kSlot, 0);
    std::memcpy(box.data.data(), slot0.data(), kSlot);

    ScBlock party;
    party.key = kPartySv;
    party.type = ScType::Object;
    party.data.assign(6 * kSlot + 1, 0);
    party.data[6 * kSlot] = 0;

    std::vector<ScBlock> blocks{box, party};
    std::vector<uint8_t> enc;
    std::string err;
    assert(SwishCrypto::encrypt(blocks, enc, &err));
    assert(SwishCrypto::isHashValid(enc));
    return enc;
}

}  // namespace

int main() {
    using pkhub::GameId;
    using pkhub::Generation;
    using pkhub::parseSwitchSave;
    using pkhub::serializeSwitchSave;

    // SwishCrypto empty round-trip
    {
        auto save = buildSvSave(makeBlankPk());
        auto parsed = parseSwitchSave(GameId::Scarlet, save);
        assert(parsed.ok);
        assert(parsed.parseImplemented);
        assert(parsed.boxes.size() == 32);
        assert(parsed.boxes[0].slot(0).empty());

        std::vector<uint8_t> out;
        std::string err;
        assert(serializeSwitchSave(GameId::Scarlet, save, parsed.party, parsed.boxes, out, &err));
        assert(SwishCrypto::isHashValid(out));
    }

    // Pokémon round-trip: species/level/shiny
    {
        auto save = buildSvSave(makePikachuPk());
        auto parsed = parseSwitchSave(GameId::Violet, save);
        assert(parsed.ok);
        assert(parsed.parseImplemented);
        auto& mon = parsed.boxes[0].slot(0);
        assert(!mon.empty());
        assert(mon.species == 25);
        assert(mon.level == 25);
        assert(mon.moves[0] == 84);

        mon.isShiny = true;
        mon.level = 50;
        mon.ivs.hp = 31;
        mon.ivs.atk = 31;
        mon.ivs.def = 31;
        mon.ivs.spa = 31;
        mon.ivs.spd = 31;
        mon.ivs.spe = 31;

        std::vector<uint8_t> out;
        std::string err;
        assert(serializeSwitchSave(GameId::Violet, save, parsed.party, parsed.boxes, out, &err));

        auto again = parseSwitchSave(GameId::Violet, out);
        assert(again.ok);
        assert(again.boxes[0].slot(0).species == 25);
        assert(again.boxes[0].slot(0).level == 50);
        assert(again.boxes[0].slot(0).isShiny);
        assert(again.boxes[0].slot(0).ivs.hp == 31);
    }

    // PK crypto self-test
    {
        auto d = makePikachuPk();
        assert(pkhub::poke_crypto8::isEncrypted(d.data(), d.size()));
        pkhub::poke_crypto8::decrypt(d.data(), d.size());
        assert(!pkhub::poke_crypto8::isEncrypted(d.data(), d.size()));
        assert(d[0x08] == 25);
        pkhub::poke_crypto8::encrypt(d.data(), d.size());
        assert(pkhub::poke_crypto8::isEncrypted(d.data(), d.size()));
    }

    std::puts("swish_roundtrip_test OK");
    return 0;
}
