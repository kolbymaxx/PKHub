#include "pkhub/backends/switch/Pk8Codec.hpp"

#include "pkhub/backends/switch/PokeCrypto8.hpp"

#include <cstring>

namespace pkhub {
namespace {

uint16_t readU16(const uint8_t* p) {
    return uint16_t(p[0] | (uint16_t(p[1]) << 8));
}
uint32_t readU32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
void writeU16(uint8_t* p, uint16_t v) {
    p[0] = uint8_t(v);
    p[1] = uint8_t(v >> 8);
}
void writeU32(uint8_t* p, uint32_t v) {
    p[0] = uint8_t(v);
    p[1] = uint8_t(v >> 8);
    p[2] = uint8_t(v >> 16);
    p[3] = uint8_t(v >> 24);
}

bool isBlankSlot(const uint8_t* data) {
    // Species internal at 0x08 (decrypted) == 0 means empty.
    return readU16(data + 0x08) == 0 && readU32(data) == 0;
}

uint32_t psv(uint32_t pid) {
    return ((pid >> 16) ^ (pid & 0xFFFFu)) >> 4;
}

uint32_t tsv(uint16_t tid, uint16_t sid) {
    return uint32_t(tid ^ sid) >> 4;
}

void setShinyPid(uint8_t* data, bool shiny) {
    const uint32_t pid = readU32(data + 0x1C);
    const uint16_t tid = readU16(data + 0x0C);
    const uint16_t sid = readU16(data + 0x0E);
    const uint32_t trainer = tsv(tid, sid);
    uint32_t newPid = pid;
    if (shiny) {
        if (psv(pid) != trainer) {
            // Force shiny by aligning low/high XOR to TSV.
            const uint16_t hi = uint16_t(pid >> 16);
            const uint16_t lo = uint16_t((hi ^ (trainer << 4)) & 0xFFF0u) | uint16_t(pid & 0xFu);
            newPid = (uint32_t(hi) << 16) | lo;
        }
    } else if (psv(pid) == trainer) {
        newPid = pid ^ 0x10000000u;
    }
    writeU32(data + 0x1C, newPid);
}

PokemonType teraFromByte(uint8_t v) {
    if (v > 18) {
        return PokemonType::None;
    }
    return static_cast<PokemonType>(v);
}

}  // namespace

Pokemon decodePk8Party(const uint8_t* data, std::size_t len, Generation gen) {
    Pokemon mon;
    if (!data || len < poke_crypto8::kPartySize) {
        return mon;
    }
    std::vector<uint8_t> buf(data, data + poke_crypto8::kPartySize);
    poke_crypto8::decryptIfEncrypted(buf);
    if (isBlankSlot(buf.data())) {
        return mon;
    }

    mon.nativeBlob = buf;
    mon.nativeGeneration = gen;
    mon.species = readU16(buf.data() + 0x08);  // SV: internal species id until converter lands
    mon.heldItem = readU16(buf.data() + 0x0A);
    mon.otId = readU16(buf.data() + 0x0C);
    mon.otSecretId = readU16(buf.data() + 0x0E);
    mon.exp = readU32(buf.data() + 0x10);
    mon.abilityId = readU16(buf.data() + 0x14);
    mon.abilitySlot = int8_t(buf[0x16] & 7);
    mon.pid = readU32(buf.data() + 0x1C);
    mon.nature = int8_t(buf[0x20]);
    mon.gender = uint8_t((buf[0x22] >> 1) & 3);
    mon.form = buf[0x24];
    mon.evs.hp = buf[0x26];
    mon.evs.atk = buf[0x27];
    mon.evs.def = buf[0x28];
    mon.evs.spe = buf[0x29];
    mon.evs.spa = buf[0x2A];
    mon.evs.spd = buf[0x2B];
    mon.moves[0] = readU16(buf.data() + 0x72);
    mon.moves[1] = readU16(buf.data() + 0x74);
    mon.moves[2] = readU16(buf.data() + 0x76);
    mon.moves[3] = readU16(buf.data() + 0x78);
    mon.pp[0] = buf[0x7A];
    mon.pp[1] = buf[0x7B];
    mon.pp[2] = buf[0x7C];
    mon.pp[3] = buf[0x7D];
    mon.ppUps[0] = buf[0x7E];
    mon.ppUps[1] = buf[0x7F];
    mon.ppUps[2] = buf[0x80];
    mon.ppUps[3] = buf[0x81];
    const uint32_t iv32 = readU32(buf.data() + 0x8C);
    mon.ivs.hp = uint8_t(iv32 & 0x1F);
    mon.ivs.atk = uint8_t((iv32 >> 5) & 0x1F);
    mon.ivs.def = uint8_t((iv32 >> 10) & 0x1F);
    mon.ivs.spe = uint8_t((iv32 >> 15) & 0x1F);
    mon.ivs.spa = uint8_t((iv32 >> 20) & 0x1F);
    mon.ivs.spd = uint8_t((iv32 >> 25) & 0x1F);
    mon.isEgg = ((iv32 >> 30) & 1) != 0;
    if (gen == Generation::Gen9) {
        mon.teraType = teraFromByte(buf[0x95] != 0x99 ? buf[0x95] : buf[0x94]);
    }
    mon.friendship = buf[0x112];
    mon.ball = buf[0x124];
    mon.level = buf[0x148] == 0 ? 1 : buf[0x148];
    mon.isShiny = psv(mon.pid) == tsv(uint16_t(mon.otId), uint16_t(mon.otSecretId));
    mon.originGame = GameId::Unknown;
    return mon;
}

bool encodePk8Party(const Pokemon& mon, std::vector<uint8_t>& out, std::string* err) {
    return patchAndEncryptPk8(mon, out, err);
}

bool patchAndEncryptPk8(const Pokemon& mon, std::vector<uint8_t>& encryptedOut, std::string* err) {
    if (mon.nativeBlob.size() < poke_crypto8::kPartySize) {
        if (err) {
            *err = "Missing native PK8/PK9 blob for lossless write";
        }
        return false;
    }
    std::vector<uint8_t> buf = mon.nativeBlob;
    if (buf.size() > poke_crypto8::kPartySize) {
        buf.resize(poke_crypto8::kPartySize);
    }
    // Ensure decrypted working copy.
    poke_crypto8::decryptIfEncrypted(buf);

    writeU16(buf.data() + 0x08, mon.species);
    writeU16(buf.data() + 0x0A, mon.heldItem);
    writeU16(buf.data() + 0x0C, uint16_t(mon.otId));
    writeU16(buf.data() + 0x0E, uint16_t(mon.otSecretId));
    writeU32(buf.data() + 0x10, mon.exp);
    writeU16(buf.data() + 0x14, mon.abilityId);
    buf[0x16] = uint8_t((buf[0x16] & ~7) | (mon.abilitySlot < 0 ? 0 : (mon.abilitySlot & 7)));
    writeU32(buf.data() + 0x1C, mon.pid);
    if (mon.nature >= 0) {
        buf[0x20] = uint8_t(mon.nature);
        buf[0x21] = uint8_t(mon.nature);
    }
    buf[0x22] = uint8_t((buf[0x22] & 0xF9) | ((mon.gender & 3) << 1));
    buf[0x24] = mon.form;
    buf[0x26] = mon.evs.hp;
    buf[0x27] = mon.evs.atk;
    buf[0x28] = mon.evs.def;
    buf[0x29] = mon.evs.spe;
    buf[0x2A] = mon.evs.spa;
    buf[0x2B] = mon.evs.spd;
    writeU16(buf.data() + 0x72, mon.moves[0]);
    writeU16(buf.data() + 0x74, mon.moves[1]);
    writeU16(buf.data() + 0x76, mon.moves[2]);
    writeU16(buf.data() + 0x78, mon.moves[3]);
    buf[0x7A] = mon.pp[0];
    buf[0x7B] = mon.pp[1];
    buf[0x7C] = mon.pp[2];
    buf[0x7D] = mon.pp[3];
    buf[0x7E] = mon.ppUps[0];
    buf[0x7F] = mon.ppUps[1];
    buf[0x80] = mon.ppUps[2];
    buf[0x81] = mon.ppUps[3];
    uint32_t iv32 = 0;
    iv32 |= (uint32_t(mon.ivs.hp) & 0x1F);
    iv32 |= (uint32_t(mon.ivs.atk) & 0x1F) << 5;
    iv32 |= (uint32_t(mon.ivs.def) & 0x1F) << 10;
    iv32 |= (uint32_t(mon.ivs.spe) & 0x1F) << 15;
    iv32 |= (uint32_t(mon.ivs.spa) & 0x1F) << 20;
    iv32 |= (uint32_t(mon.ivs.spd) & 0x1F) << 25;
    if (mon.isEgg) {
        iv32 |= 0x40000000u;
    }
    // preserve nickname flag from prior blob
    iv32 |= (readU32(buf.data() + 0x8C) & 0x80000000u);
    writeU32(buf.data() + 0x8C, iv32);
    if (mon.nativeGeneration == Generation::Gen9 || mon.teraType != PokemonType::None) {
        if (mon.teraType != PokemonType::None) {
            buf[0x95] = uint8_t(mon.teraType);
        }
    }
    buf[0x112] = mon.friendship;
    buf[0x124] = mon.ball;
    buf[0x148] = mon.level == 0 ? 1 : mon.level;

    setShinyPid(buf.data(), mon.isShiny);
    poke_crypto8::refreshChecksum(buf.data());
    poke_crypto8::encrypt(buf.data(), buf.size());
    encryptedOut = std::move(buf);
    return true;
}

}  // namespace pkhub
