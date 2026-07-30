#include "pkhub/core/hub/HubPokemonCodec.hpp"

#include "pkhub/core/hub/HubFormat.hpp"

#include <string>
#include <vector>

namespace pkhub::hub_codec {
namespace {

void appendU8(std::vector<uint8_t>& out, uint8_t v) {
    out.push_back(v);
}

void appendU16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

void appendU32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

void appendBytes(std::vector<uint8_t>& out, const void* data, std::size_t len) {
    const auto* p = static_cast<const uint8_t*>(data);
    out.insert(out.end(), p, p + len);
}

void appendStats(std::vector<uint8_t>& out, const Stats& s) {
    appendU8(out, s.hp);
    appendU8(out, s.atk);
    appendU8(out, s.def);
    appendU8(out, s.spa);
    appendU8(out, s.spd);
    appendU8(out, s.spe);
}

void appendString(std::vector<uint8_t>& out, const std::string& s) {
    if (s.size() > 0xFFFF) {
        appendU16(out, 0xFFFF);
        appendBytes(out, s.data(), 0xFFFF);
        return;
    }
    appendU16(out, static_cast<uint16_t>(s.size()));
    appendBytes(out, s.data(), s.size());
}

bool readU8(const uint8_t*& p, const uint8_t* end, uint8_t& v) {
    if (p >= end) {
        return false;
    }
    v = *p++;
    return true;
}

bool readU16(const uint8_t*& p, const uint8_t* end, uint16_t& v) {
    if (end - p < 2) {
        return false;
    }
    v = static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
    p += 2;
    return true;
}

bool readU32(const uint8_t*& p, const uint8_t* end, uint32_t& v) {
    if (end - p < 4) {
        return false;
    }
    v = static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
        (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
    p += 4;
    return true;
}

bool readStats(const uint8_t*& p, const uint8_t* end, Stats& s) {
    return readU8(p, end, s.hp) && readU8(p, end, s.atk) && readU8(p, end, s.def) &&
           readU8(p, end, s.spa) && readU8(p, end, s.spd) && readU8(p, end, s.spe);
}

bool readString(const uint8_t*& p, const uint8_t* end, std::string& s) {
    uint16_t len = 0;
    if (!readU16(p, end, len)) {
        return false;
    }
    if (static_cast<std::size_t>(end - p) < len) {
        return false;
    }
    s.assign(reinterpret_cast<const char*>(p), len);
    p += len;
    return true;
}

}  // namespace

std::vector<uint8_t> encodePokemon(const Pokemon& mon) {
    std::vector<uint8_t> out;
    out.reserve(96 + mon.nickname.size() + mon.otName.size() + mon.nativeBlob.size());

    appendU16(out, hub_format::kCodecVersion);
    appendU16(out, mon.species);
    appendU8(out, mon.form);

    uint8_t flags = 0;
    if (mon.isShiny) {
        flags |= 0x01;
    }
    if (mon.isEgg) {
        flags |= 0x02;
    }
    if (mon.isAlpha) {
        flags |= 0x04;
    }
    appendU8(out, flags);
    appendU8(out, mon.level);
    appendU8(out, mon.nature < 0 ? uint8_t{255} : static_cast<uint8_t>(mon.nature));
    appendU16(out, mon.abilityId);
    appendU8(out, mon.abilitySlot < 0 ? uint8_t{255} : static_cast<uint8_t>(mon.abilitySlot));
    appendU8(out, mon.gender);
    appendU8(out, mon.ball);
    appendU16(out, mon.heldItem);
    appendU8(out, static_cast<uint8_t>(mon.teraType));
    appendU32(out, mon.pid);
    appendU32(out, mon.otId);
    appendU32(out, mon.otSecretId);
    appendU32(out, mon.exp);
    appendU8(out, mon.friendship);
    appendStats(out, mon.ivs);
    appendStats(out, mon.evs);
    for (uint16_t move : mon.moves) {
        appendU16(out, move);
    }
    for (uint8_t v : mon.pp) {
        appendU8(out, v);
    }
    for (uint8_t v : mon.ppUps) {
        appendU8(out, v);
    }
    appendString(out, mon.nickname);
    appendString(out, mon.otName);

    const auto nativeLen =
        mon.nativeBlob.size() > 0xFFFF ? std::size_t{0xFFFF} : mon.nativeBlob.size();
    appendU16(out, static_cast<uint16_t>(nativeLen));
    if (nativeLen > 0) {
        appendBytes(out, mon.nativeBlob.data(), nativeLen);
    }
    return out;
}

bool decodePokemon(const uint8_t* data, std::size_t len, Pokemon& out) {
    if (data == nullptr || len == 0) {
        return false;
    }
    const uint8_t* p = data;
    const uint8_t* end = data + len;

    uint16_t codecVersion = 0;
    if (!readU16(p, end, codecVersion) || codecVersion != hub_format::kCodecVersion) {
        return false;
    }

    Pokemon mon;
    uint8_t flags = 0;
    uint8_t natureRaw = 0;
    uint8_t abilitySlotRaw = 0;
    uint8_t teraRaw = 0;

    if (!readU16(p, end, mon.species) || !readU8(p, end, mon.form) || !readU8(p, end, flags) ||
        !readU8(p, end, mon.level) || !readU8(p, end, natureRaw) ||
        !readU16(p, end, mon.abilityId) || !readU8(p, end, abilitySlotRaw) ||
        !readU8(p, end, mon.gender) || !readU8(p, end, mon.ball) ||
        !readU16(p, end, mon.heldItem) || !readU8(p, end, teraRaw) || !readU32(p, end, mon.pid) ||
        !readU32(p, end, mon.otId) || !readU32(p, end, mon.otSecretId) ||
        !readU32(p, end, mon.exp) || !readU8(p, end, mon.friendship) ||
        !readStats(p, end, mon.ivs) || !readStats(p, end, mon.evs)) {
        return false;
    }

    mon.isShiny = (flags & 0x01) != 0;
    mon.isEgg = (flags & 0x02) != 0;
    mon.isAlpha = (flags & 0x04) != 0;
    mon.nature = natureRaw == 255 ? int8_t{-1} : static_cast<int8_t>(natureRaw);
    mon.abilitySlot = abilitySlotRaw == 255 ? int8_t{-1} : static_cast<int8_t>(abilitySlotRaw);
    mon.teraType = static_cast<PokemonType>(teraRaw);

    for (auto& move : mon.moves) {
        if (!readU16(p, end, move)) {
            return false;
        }
    }
    for (auto& v : mon.pp) {
        if (!readU8(p, end, v)) {
            return false;
        }
    }
    for (auto& v : mon.ppUps) {
        if (!readU8(p, end, v)) {
            return false;
        }
    }
    if (!readString(p, end, mon.nickname) || !readString(p, end, mon.otName)) {
        return false;
    }

    uint16_t nativeLen = 0;
    if (!readU16(p, end, nativeLen)) {
        return false;
    }
    if (nativeLen > 0) {
        if (static_cast<std::size_t>(end - p) < nativeLen) {
            return false;
        }
        mon.nativeBlob.assign(p, p + nativeLen);
        p += nativeLen;
    }

    out = std::move(mon);
    return true;
}

}  // namespace pkhub::hub_codec
