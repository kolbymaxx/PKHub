#include "pkhub/backends/raw/GbaGen3.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace pkhub::gba {
namespace {

// Substructure order for personality % 24 → G,A,E,M indices (0..3).
// Public Gen 3 shuffle table (Bulbapedia / community docs).
constexpr uint8_t kSubstructOrder[24][4] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 2, 3, 1}, {0, 3, 1, 2}, {0, 3, 2, 1},
    {1, 0, 2, 3}, {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 2, 3, 0}, {1, 3, 0, 2}, {1, 3, 2, 0},
    {2, 0, 1, 3}, {2, 0, 3, 1}, {2, 1, 0, 3}, {2, 1, 3, 0}, {2, 3, 0, 1}, {2, 3, 1, 0},
    {3, 0, 1, 2}, {3, 0, 2, 1}, {3, 1, 0, 2}, {3, 1, 2, 0}, {3, 2, 0, 1}, {3, 2, 1, 0},
};

constexpr int kSubGrowth = 0;
constexpr int kSubAttacks = 1;
constexpr int kSubEvs = 2;
constexpr int kSubMisc = 3;

uint16_t read16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

uint32_t read32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

void write16(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
}

void write32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFF);
}

bool isShinyPid(uint32_t pid, uint16_t tid, uint16_t sid) {
    const uint16_t hid = static_cast<uint16_t>(pid >> 16);
    const uint16_t lid = static_cast<uint16_t>(pid & 0xFFFF);
    return (tid ^ sid ^ hid ^ lid) < 8;
}

uint16_t boxMonChecksum(const uint8_t* encrypted48) {
    uint32_t sum = 0;
    for (int i = 0; i < 24; ++i) {
        sum += read16(encrypted48 + i * 2);
    }
    return static_cast<uint16_t>(sum & 0xFFFF);
}

void cryptRegion(uint8_t* data48, uint32_t key) {
    for (int i = 0; i < 12; ++i) {
        const uint32_t v = read32(data48 + i * 4) ^ key;
        write32(data48 + i * 4, v);
    }
}

void decryptSubstructs(const uint8_t* src80, uint8_t outSubs[4][12]) {
    uint8_t enc[48];
    std::memcpy(enc, src80 + 32, 48);
    const uint32_t pid = read32(src80 + 0);
    const uint32_t otId = read32(src80 + 4);
    cryptRegion(enc, pid ^ otId);

    const auto& order = kSubstructOrder[pid % 24];
    for (int slot = 0; slot < 4; ++slot) {
        std::memcpy(outSubs[order[slot]], enc + slot * 12, 12);
    }
}

void encryptFromSubstructs(uint32_t pid, uint32_t otId, const uint8_t subs[4][12], uint8_t* dst80) {
    uint8_t enc[48]{};
    const auto& order = kSubstructOrder[pid % 24];
    for (int slot = 0; slot < 4; ++slot) {
        std::memcpy(enc + slot * 12, subs[order[slot]], 12);
    }
    const uint16_t chk = boxMonChecksum(enc);
    write16(dst80 + 0x1C, chk);
    write16(dst80 + 0x1E, 0);
    cryptRegion(enc, pid ^ otId);
    std::memcpy(dst80 + 32, enc, 48);
}

bool speciesPlausible(uint16_t species) {
    return species >= 1 && species <= 411;  // Gen 3 national dex incl. Glitches up to Unown forms range; keep soft
}

bool looksLikeParty(const uint8_t* teamSection, std::size_t countOff, std::size_t partyOff) {
    const uint32_t count = read32(teamSection + countOff);
    if (count == 0 || count > 6) {
        return false;
    }
    const uint8_t* mon = teamSection + partyOff;
    const uint32_t pid = read32(mon);
    const uint32_t otId = read32(mon + 4);
    if (pid == 0 && otId == 0) {
        // Empty first slot with count>0 is suspicious
        return false;
    }
    uint8_t subs[4][12]{};
    decryptSubstructs(mon, subs);
    const uint16_t species = read16(subs[kSubGrowth] + 0);
    return speciesPlausible(species);
}

// Minimal medium-slow style exp→level for display when only box data exists.
uint8_t levelFromExpFallback(uint32_t exp) {
    // Rough Gen 3 medium-slow: exp ≈ (6/5)n^3 - 15n^2 + 100n - 140
    for (uint8_t lv = 100; lv >= 1; --lv) {
        const int n = lv;
        const int64_t need = (6 * n * n * n) / 5 - 15 * n * n + 100 * n - 140;
        if (need < 0) {
            continue;
        }
        if (exp >= static_cast<uint32_t>(need)) {
            return lv;
        }
    }
    return 1;
}

uint32_t expForLevelFallback(uint8_t level) {
    if (level <= 1) {
        return 0;
    }
    const int n = std::min<int>(level, 100);
    const int64_t need = (6 * n * n * n) / 5 - 15 * n * n + 100 * n - 140;
    return need > 0 ? static_cast<uint32_t>(need) : 0;
}

struct SectionView {
    const uint8_t* data = nullptr;  // points at section start (0x1000)
    uint16_t id = 0xFFFF;
    uint32_t saveIndex = 0;
    bool valid = false;
};

SectionView readSectionFooter(const uint8_t* section) {
    SectionView v;
    v.data = section;
    v.id = read16(section + 0x0FF4);
    const uint32_t sig = read32(section + 0x0FF8);
    v.saveIndex = read32(section + 0x0FFC);
    v.valid = (sig == kSectionSignature) && (v.id < kSectionsPerSlot);
    return v;
}

struct SlotInfo {
    std::array<const uint8_t*, kSectionsPerSlot> byId{};
    uint32_t saveIndex = 0;
    int validCount = 0;
};

SlotInfo scanSlot(const uint8_t* slotBase) {
    SlotInfo info;
    info.byId.fill(nullptr);
    for (std::size_t i = 0; i < kSectionsPerSlot; ++i) {
        const uint8_t* sec = slotBase + i * kSectionSize;
        const SectionView v = readSectionFooter(sec);
        if (!v.valid) {
            continue;
        }
        info.byId[v.id] = sec;
        info.saveIndex = v.saveIndex;
        ++info.validCount;
    }
    return info;
}

GameId detectGame(const uint8_t* trainerSec, const uint8_t* teamSec) {
    if (!trainerSec || !teamSec) {
        return GameId::Unknown;
    }

    const bool frlgParty = looksLikeParty(teamSec, 0x0034, 0x0038);
    const bool rsParty = looksLikeParty(teamSec, 0x0234, 0x0238);

    // Trainer Info 0xAC: RS game code (0=Sapphire, 1=Ruby); Emerald stores security key;
    // FRLG also stores a version code (0=LeafGreen, 1=FireRed) at the same offset.
    const uint32_t code = read32(trainerSec + 0xAC);

    if (frlgParty && !rsParty) {
        return (code == 0) ? GameId::LeafGreen : GameId::FireRed;
    }
    if (rsParty) {
        if (code == 0) {
            return GameId::Sapphire;
        }
        if (code == 1) {
            return GameId::Ruby;
        }
        // Emerald overwrites 0xAC with the security key (typically not just 0/1).
        return GameId::Emerald;
    }
    if (frlgParty) {
        return (code == 0) ? GameId::LeafGreen : GameId::FireRed;
    }

    // Soft fallback from code alone.
    if (code == 0) {
        return GameId::Sapphire;
    }
    if (code == 1) {
        return GameId::Ruby;
    }
    if (code > 1) {
        return GameId::Emerald;
    }
    return GameId::Unknown;
}

void fillPokemonFromSubs(const uint8_t* src80, const uint8_t subs[4][12], Pokemon& out) {
    out.clear();
    const uint32_t pid = read32(src80 + 0);
    const uint32_t otId = read32(src80 + 4);
    const uint16_t tid = static_cast<uint16_t>(otId & 0xFFFF);
    const uint16_t sid = static_cast<uint16_t>((otId >> 16) & 0xFFFF);

    const uint16_t species = read16(subs[kSubGrowth] + 0);
    if (species == 0) {
        return;
    }

    out.species = species;
    out.heldItem = read16(subs[kSubGrowth] + 2);
    out.exp = read32(subs[kSubGrowth] + 4);
    const uint8_t ppBonuses = subs[kSubGrowth][8];
    out.friendship = subs[kSubGrowth][9];
    out.ppUps = {
        static_cast<uint8_t>(ppBonuses & 3),
        static_cast<uint8_t>((ppBonuses >> 2) & 3),
        static_cast<uint8_t>((ppBonuses >> 4) & 3),
        static_cast<uint8_t>((ppBonuses >> 6) & 3),
    };

    out.moves = {
        read16(subs[kSubAttacks] + 0),
        read16(subs[kSubAttacks] + 2),
        read16(subs[kSubAttacks] + 4),
        read16(subs[kSubAttacks] + 6),
    };
    out.pp = {
        subs[kSubAttacks][8],
        subs[kSubAttacks][9],
        subs[kSubAttacks][10],
        subs[kSubAttacks][11],
    };

    out.evs.hp = subs[kSubEvs][0];
    out.evs.atk = subs[kSubEvs][1];
    out.evs.def = subs[kSubEvs][2];
    out.evs.spe = subs[kSubEvs][3];
    out.evs.spa = subs[kSubEvs][4];
    out.evs.spd = subs[kSubEvs][5];

    out.metLocation = subs[kSubMisc][1];
    const uint16_t origins = read16(subs[kSubMisc] + 2);
    out.metLevel = static_cast<uint8_t>(origins & 0x7F);
    out.ball = static_cast<uint8_t>((origins >> 11) & 0x0F);

    const uint32_t ivEggAb = read32(subs[kSubMisc] + 4);
    out.ivs.hp = static_cast<uint8_t>(ivEggAb & 0x1F);
    out.ivs.atk = static_cast<uint8_t>((ivEggAb >> 5) & 0x1F);
    out.ivs.def = static_cast<uint8_t>((ivEggAb >> 10) & 0x1F);
    out.ivs.spe = static_cast<uint8_t>((ivEggAb >> 15) & 0x1F);
    out.ivs.spa = static_cast<uint8_t>((ivEggAb >> 20) & 0x1F);
    out.ivs.spd = static_cast<uint8_t>((ivEggAb >> 25) & 0x1F);
    out.isEgg = ((ivEggAb >> 30) & 1) != 0;
    out.abilitySlot = static_cast<int8_t>((ivEggAb >> 31) & 1);

    out.pid = pid;
    out.otId = tid;
    out.otSecretId = sid;
    out.isShiny = isShinyPid(pid, tid, sid);
    out.nature = static_cast<int8_t>(pid % 25);
    out.level = levelFromExpFallback(out.exp);
    out.nativeGeneration = Generation::Gen3;
    out.nativeBlob.assign(src80, src80 + kBoxMonSize);
}

std::vector<uint8_t> concatPc(const SlotInfo& slot) {
    // Sections 5–13 hold the PC buffer. Concatenate full 0xF80 data regions
    // (footer starts at 0xFF4; usable payload is treated as up to 0xF80).
    constexpr std::size_t kPayload = 0xF80;
    std::vector<uint8_t> pc;
    pc.reserve(9 * kPayload);
    for (uint16_t id = 5; id <= 13; ++id) {
        if (!slot.byId[id]) {
            pc.insert(pc.end(), kPayload, 0);
            continue;
        }
        pc.insert(pc.end(), slot.byId[id], slot.byId[id] + kPayload);
    }
    return pc;
}

// Public Gen 3 section payload sizes used for checksum (pokeemerald / community docs).
constexpr uint16_t kSectionChecksumSize[kSectionsPerSlot] = {
    0xF24, 0xF80, 0xF80, 0xF80, 0xF08, 0xF80, 0xF80,
    0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0x7D0,
};

uint16_t sectionChecksum(const uint8_t* section, uint16_t byteSize) {
    uint32_t sum = 0;
    const std::size_t words = byteSize / 4;
    for (std::size_t i = 0; i < words; ++i) {
        sum += read32(section + i * 4);
    }
    return static_cast<uint16_t>((sum + (sum >> 16)) & 0xFFFF);
}

void writeSectionFooter(uint8_t* section, uint16_t id, uint32_t saveIndex) {
    const uint16_t chkSize = kSectionChecksumSize[id < kSectionsPerSlot ? id : 0];
    const uint16_t chk = sectionChecksum(section, chkSize);
    write16(section + 0x0FF4, id);
    write16(section + 0x0FF6, chk);
    write32(section + 0x0FF8, kSectionSignature);
    write32(section + 0x0FFC, saveIndex);
}

struct MutableSlot {
    std::array<uint8_t*, kSectionsPerSlot> byId{};
    uint32_t saveIndex = 0;
    int validCount = 0;
    std::size_t slotBaseOffset = 0;
};

MutableSlot scanMutableSlot(uint8_t* saveBase, std::size_t slotBaseOffset) {
    MutableSlot info;
    info.byId.fill(nullptr);
    info.slotBaseOffset = slotBaseOffset;
    uint8_t* slotBase = saveBase + slotBaseOffset;
    for (std::size_t i = 0; i < kSectionsPerSlot; ++i) {
        uint8_t* sec = slotBase + i * kSectionSize;
        const SectionView v = readSectionFooter(sec);
        if (!v.valid) {
            continue;
        }
        info.byId[v.id] = sec;
        info.saveIndex = v.saveIndex;
        ++info.validCount;
    }
    return info;
}

void partyOffsetsForGame(GameId game, std::size_t& countOff, std::size_t& partyOff) {
    const bool frlg = (game == GameId::FireRed || game == GameId::LeafGreen);
    if (frlg) {
        countOff = 0x0034;
        partyOff = 0x0038;
    } else {
        countOff = 0x0234;
        partyOff = 0x0238;
    }
}

bool writePokemonBytes(const Pokemon& mon, bool party, uint8_t* dst, std::size_t dstSize) {
    if (!dst || dstSize < (party ? kPartyMonSize : kBoxMonSize)) {
        return false;
    }
    if (mon.empty()) {
        std::memset(dst, 0, party ? kPartyMonSize : kBoxMonSize);
        return true;
    }
    // Prefer re-encode from unified fields so editor changes (shiny/level/etc.) persist.
    // nativeBlob is only used when encode is skipped for empty — see above.
    if (party) {
        return encodePartyMon(mon, dst);
    }
    return encodeBoxMon(mon, dst);
}

}  // namespace

bool decodeBoxMon(const uint8_t* src80, Pokemon& out) {
    out.clear();
    if (!src80) {
        return false;
    }
    // Empty slot: all-zero personality+ot and species will be 0 after decrypt of zeros.
    bool allZero = true;
    for (int i = 0; i < static_cast<int>(kBoxMonSize); ++i) {
        if (src80[i] != 0) {
            allZero = false;
            break;
        }
    }
    if (allZero) {
        return false;
    }

    uint8_t subs[4][12]{};
    decryptSubstructs(src80, subs);
    const uint16_t species = read16(subs[kSubGrowth] + 0);
    if (species == 0) {
        return false;
    }
    fillPokemonFromSubs(src80, subs, out);
    return !out.empty();
}

bool decodePartyMon(const uint8_t* src100, Pokemon& out) {
    if (!decodeBoxMon(src100, out)) {
        return false;
    }
    out.level = src100[0x54];
    if (out.level == 0) {
        out.level = 1;
    }
    out.nativeBlob.assign(src100, src100 + kPartyMonSize);
    return true;
}

bool encodeBoxMon(const Pokemon& in, uint8_t* dst80) {
    if (!dst80) {
        return false;
    }
    std::memset(dst80, 0, kBoxMonSize);
    if (in.empty()) {
        return true;
    }

    uint32_t pid = in.pid;
    uint16_t tid = static_cast<uint16_t>(in.otId & 0xFFFF);
    uint16_t sid = static_cast<uint16_t>(in.otSecretId & 0xFFFF);

    if (pid == 0) {
        // Deterministic placeholder personality from species + tid.
        pid = (static_cast<uint32_t>(in.species) << 16) ^ tid ^ 0x1234u;
        if (pid == 0) {
            pid = 1;
        }
    }

    // Adjust SID for requested shiny state (keep TID/PID).
    if (in.isShiny != isShinyPid(pid, tid, sid)) {
        const uint16_t hid = static_cast<uint16_t>(pid >> 16);
        const uint16_t lid = static_cast<uint16_t>(pid & 0xFFFF);
        if (in.isShiny) {
            sid = static_cast<uint16_t>(tid ^ hid ^ lid);  // xor = 0 → shiny
        } else {
            // Force non-shiny: flip high bit of SID relative to shiny xor.
            const uint16_t xorVal = static_cast<uint16_t>(tid ^ hid ^ lid);
            sid = static_cast<uint16_t>(xorVal ^ 0x8000);
        }
    }

    // Optional nature override via PID low bits (best-effort; may perturb shiny slightly — we re-check).
    if (in.nature >= 0 && in.nature < 25 && static_cast<int>(pid % 25) != in.nature) {
        pid = (pid / 25) * 25 + static_cast<uint32_t>(in.nature);
        if (pid == 0) {
            pid = static_cast<uint32_t>(in.nature);
        }
        if (in.isShiny != isShinyPid(pid, tid, sid)) {
            const uint16_t hid = static_cast<uint16_t>(pid >> 16);
            const uint16_t lid = static_cast<uint16_t>(pid & 0xFFFF);
            if (in.isShiny) {
                sid = static_cast<uint16_t>(tid ^ hid ^ lid);
            } else {
                sid = static_cast<uint16_t>((tid ^ hid ^ lid) ^ 0x8000);
            }
        }
    }

    const uint32_t otId = (static_cast<uint32_t>(sid) << 16) | tid;
    write32(dst80 + 0, pid);
    write32(dst80 + 4, otId);
    // Nickname / OT left zero (blank); language English.
    dst80[0x12] = 2;  // English

    uint8_t subs[4][12]{};
    write16(subs[kSubGrowth] + 0, in.species);
    write16(subs[kSubGrowth] + 2, in.heldItem);
    uint32_t exp = in.exp;
    if (exp == 0 && in.level > 1) {
        exp = expForLevelFallback(in.level);
    }
    write32(subs[kSubGrowth] + 4, exp);
    const uint8_t ppBonuses = static_cast<uint8_t>(
        (in.ppUps[0] & 3) | ((in.ppUps[1] & 3) << 2) | ((in.ppUps[2] & 3) << 4) |
        ((in.ppUps[3] & 3) << 6));
    subs[kSubGrowth][8] = ppBonuses;
    subs[kSubGrowth][9] = in.friendship;

    write16(subs[kSubAttacks] + 0, in.moves[0]);
    write16(subs[kSubAttacks] + 2, in.moves[1]);
    write16(subs[kSubAttacks] + 4, in.moves[2]);
    write16(subs[kSubAttacks] + 6, in.moves[3]);
    subs[kSubAttacks][8] = in.pp[0];
    subs[kSubAttacks][9] = in.pp[1];
    subs[kSubAttacks][10] = in.pp[2];
    subs[kSubAttacks][11] = in.pp[3];

    subs[kSubEvs][0] = in.evs.hp;
    subs[kSubEvs][1] = in.evs.atk;
    subs[kSubEvs][2] = in.evs.def;
    subs[kSubEvs][3] = in.evs.spe;
    subs[kSubEvs][4] = in.evs.spa;
    subs[kSubEvs][5] = in.evs.spd;

    subs[kSubMisc][0] = 0;  // pokerus
    subs[kSubMisc][1] = static_cast<uint8_t>(in.metLocation & 0xFF);
    uint16_t origins = static_cast<uint16_t>(in.metLevel & 0x7F);
    // game 4 bits (bits 7-10): leave 0; ball bits 11-14; OT gender bit 15
    origins |= static_cast<uint16_t>((in.ball & 0x0F) << 11);
    write16(subs[kSubMisc] + 2, origins);

    uint32_t ivEggAb = 0;
    ivEggAb |= (in.ivs.hp & 0x1F);
    ivEggAb |= (static_cast<uint32_t>(in.ivs.atk & 0x1F) << 5);
    ivEggAb |= (static_cast<uint32_t>(in.ivs.def & 0x1F) << 10);
    ivEggAb |= (static_cast<uint32_t>(in.ivs.spe & 0x1F) << 15);
    ivEggAb |= (static_cast<uint32_t>(in.ivs.spa & 0x1F) << 20);
    ivEggAb |= (static_cast<uint32_t>(in.ivs.spd & 0x1F) << 25);
    if (in.isEgg) {
        ivEggAb |= (1u << 30);
    }
    if (in.abilitySlot > 0) {
        ivEggAb |= (1u << 31);
    }
    write32(subs[kSubMisc] + 4, ivEggAb);

    encryptFromSubstructs(pid, otId, subs, dst80);
    return true;
}

bool encodePartyMon(const Pokemon& in, uint8_t* dst100) {
    if (!dst100) {
        return false;
    }
    std::memset(dst100, 0, kPartyMonSize);
    if (in.empty()) {
        return true;
    }
    if (!encodeBoxMon(in, dst100)) {
        return false;
    }
    // Battle stats trailer (0x50..0x63). Phase 1: minimal valid-looking values.
    write32(dst100 + 0x50, 0);  // status
    dst100[0x54] = in.level == 0 ? 1 : in.level;
    dst100[0x55] = 0;
    const uint16_t hp = 20;  // placeholder; full base-stat calc later
    write16(dst100 + 0x56, hp);
    write16(dst100 + 0x58, hp);
    write16(dst100 + 0x5A, 10);
    write16(dst100 + 0x5C, 10);
    write16(dst100 + 0x5E, 10);
    write16(dst100 + 0x60, 10);
    write16(dst100 + 0x62, 10);
    return true;
}

bool writeSave(std::vector<uint8_t>& data,
               GameId game,
               const Party& party,
               const std::vector<Box>& boxes,
               std::string* err) {
    auto fail = [&](const char* msg) {
        if (err) {
            *err = msg;
        }
        return false;
    };

    if (data.size() < kGbaSaveSize) {
        return fail("Save too small for Gen 3 GBA write-back");
    }
    if (boxes.size() < kBoxCount) {
        return fail("Expected 14 PC boxes");
    }

    uint8_t* base = data.data();
    MutableSlot slotA = scanMutableSlot(base, 0);
    MutableSlot slotB = scanMutableSlot(base, kSlotSize);

    MutableSlot* active = nullptr;
    if (slotA.validCount == 0 && slotB.validCount == 0) {
        return fail("No valid Gen 3 save sections found");
    }
    if (slotA.validCount == 0) {
        active = &slotB;
    } else if (slotB.validCount == 0) {
        active = &slotA;
    } else {
        active = (slotB.saveIndex > slotA.saveIndex) ? &slotB : &slotA;
    }

    // Ensure required sections exist.
    if (!active->byId[1]) {
        return fail("Missing Team/Items section");
    }
    for (uint16_t id = 5; id <= 13; ++id) {
        if (!active->byId[id]) {
            return fail("Missing PC storage section");
        }
    }

    const uint32_t newSaveIndex = active->saveIndex + 1;

    // --- Party ---
    std::size_t countOff = 0x0234;
    std::size_t partyOff = 0x0238;
    partyOffsetsForGame(game, countOff, partyOff);

    uint8_t* team = active->byId[1];
    uint32_t count = 0;
    for (std::size_t i = 0; i < kPartySlots; ++i) {
        const Pokemon& mon = party.slot(i);
        uint8_t* dst = team + partyOff + i * kPartyMonSize;
        if (!writePokemonBytes(mon, true, dst, kPartyMonSize)) {
            return fail("Failed to encode party Pokémon");
        }
        if (!mon.empty()) {
            count = static_cast<uint32_t>(i + 1);
        }
    }
    // Compact rule: count = last occupied index + 1 (standard in-game layout).
    write32(team + countOff, count);

    // --- PC boxes ---
    constexpr std::size_t kPayload = 0xF80;
    constexpr std::size_t kPokemonStart = 4;
    std::vector<uint8_t> pc(9 * kPayload, 0);
    // Preserve currentBox from existing PC.
    {
        std::vector<uint8_t> oldPc;
        oldPc.reserve(9 * kPayload);
        for (uint16_t id = 5; id <= 13; ++id) {
            oldPc.insert(oldPc.end(), active->byId[id], active->byId[id] + kPayload);
        }
        if (oldPc.size() >= 4) {
            std::memcpy(pc.data(), oldPc.data(), 4);
        }
    }

    for (std::size_t box = 0; box < kBoxCount; ++box) {
        for (std::size_t slot = 0; slot < kSlotsPerBox; ++slot) {
            const std::size_t off = kPokemonStart + (box * kSlotsPerBox + slot) * kBoxMonSize;
            if (off + kBoxMonSize > pc.size()) {
                return fail("PC buffer overflow");
            }
            const Pokemon& mon = boxes[box].slot(slot);
            if (!writePokemonBytes(mon, false, pc.data() + off, kBoxMonSize)) {
                return fail("Failed to encode box Pokémon");
            }
        }
    }

    // Scatter PC back into sections 5–13.
    for (uint16_t id = 5; id <= 13; ++id) {
        const std::size_t chunk = static_cast<std::size_t>(id - 5) * kPayload;
        std::memcpy(active->byId[id], pc.data() + chunk, kPayload);
    }

    // Re-checksum and bump saveIndex on every section in the active slot.
    for (uint16_t id = 0; id < kSectionsPerSlot; ++id) {
        if (!active->byId[id]) {
            continue;
        }
        writeSectionFooter(active->byId[id], id, newSaveIndex);
    }

    if (err) {
        err->clear();
    }
    return true;
}

GbaProbeResult probeSave(const std::vector<uint8_t>& data) {
    GbaProbeResult out;
    if (data.size() < kGbaSaveSize) {
        return out;
    }
    const uint8_t* base = data.data();
    const SlotInfo slotA = scanSlot(base + 0);
    const SlotInfo slotB = scanSlot(base + kSlotSize);
    if (slotA.validCount == 0 && slotB.validCount == 0) {
        return out;
    }
    const SlotInfo* active = nullptr;
    if (slotA.validCount == 0) {
        active = &slotB;
    } else if (slotB.validCount == 0) {
        active = &slotA;
    } else {
        active = (slotB.saveIndex > slotA.saveIndex) ? &slotB : &slotA;
    }
    out.looksLikeGba = true;
    out.validSections = active->validCount;
    out.saveIndex = active->saveIndex;
    out.game = detectGame(active->byId[0], active->byId[1]);
    return out;
}

GbaParseResult parseSave(const std::vector<uint8_t>& data) {
    GbaParseResult result;
    result.party = Party{};
    result.boxes.assign(kBoxCount, Box{kSlotsPerBox});
    for (std::size_t i = 0; i < result.boxes.size(); ++i) {
        result.boxes[i].setName("Box " + std::to_string(i + 1));
    }

    if (data.size() < kGbaSaveSize) {
        result.message = "Save too small for Gen 3 GBA (need 128KB)";
        return result;
    }

    const GbaProbeResult probe = probeSave(data);
    if (!probe.looksLikeGba) {
        result.message = "No valid Gen 3 save sections found";
        return result;
    }

    const uint8_t* base = data.data();
    const SlotInfo slotA = scanSlot(base + 0);
    const SlotInfo slotB = scanSlot(base + kSlotSize);
    const SlotInfo* active = nullptr;
    if (slotA.validCount == 0) {
        active = &slotB;
    } else if (slotB.validCount == 0) {
        active = &slotA;
    } else {
        active = (slotB.saveIndex > slotA.saveIndex) ? &slotB : &slotA;
    }

    const uint8_t* trainer = active->byId[0];
    const uint8_t* team = active->byId[1];
    result.game = probe.game != GameId::Unknown ? probe.game : detectGame(trainer, team);

    // --- Party ---
    if (team) {
        std::size_t countOff = 0x0234;
        std::size_t partyOff = 0x0238;
        const bool frlg = (result.game == GameId::FireRed || result.game == GameId::LeafGreen);
        if (frlg || looksLikeParty(team, 0x0034, 0x0038)) {
            if (frlg || !looksLikeParty(team, 0x0234, 0x0238)) {
                countOff = 0x0034;
                partyOff = 0x0038;
            }
        }
        // If detection ambiguous, try RS/E first then FRLG.
        if (!looksLikeParty(team, countOff, partyOff)) {
            if (looksLikeParty(team, 0x0234, 0x0238)) {
                countOff = 0x0234;
                partyOff = 0x0238;
            } else if (looksLikeParty(team, 0x0034, 0x0038)) {
                countOff = 0x0034;
                partyOff = 0x0038;
            }
        }

        uint32_t count = read32(team + countOff);
        if (count > 6) {
            count = 0;
        }
        for (uint32_t i = 0; i < count && i < 6; ++i) {
            Pokemon mon;
            if (decodePartyMon(team + partyOff + i * kPartyMonSize, mon)) {
                mon.originGame = result.game;
                result.party.slot(i) = std::move(mon);
            }
        }
    }

    // --- PC boxes ---
    const auto pc = concatPc(*active);
    constexpr std::size_t kPokemonStart = 4;  // after currentBox u32
    constexpr std::size_t kPokemonBytes = kBoxCount * kSlotsPerBox * kBoxMonSize;
    if (pc.size() >= kPokemonStart + kPokemonBytes) {
        for (std::size_t box = 0; box < kBoxCount; ++box) {
            for (std::size_t slot = 0; slot < kSlotsPerBox; ++slot) {
                const std::size_t off =
                    kPokemonStart + (box * kSlotsPerBox + slot) * kBoxMonSize;
                Pokemon mon;
                if (decodeBoxMon(pc.data() + off, mon)) {
                    mon.originGame = result.game;
                    result.boxes[box].slot(slot) = std::move(mon);
                }
            }
        }
    }

    result.ok = true;
    result.message = "Parsed Gen 3 save";
    return result;
}

}  // namespace pkhub::gba
