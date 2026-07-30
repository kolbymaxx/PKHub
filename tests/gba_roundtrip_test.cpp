#include "pkhub/backends/raw/GbaGen3.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    using pkhub::Pokemon;
    using pkhub::gba::decodeBoxMon;
    using pkhub::gba::encodeBoxMon;

    Pokemon src;
    src.species = 25;  // Pikachu
    src.pid = 0x0A0B0C0D;
    src.otId = 12345;
    src.otSecretId = 54321;
    src.level = 50;
    src.exp = 125000;
    src.friendship = 200;
    src.isShiny = true;
    src.ivs = {31, 30, 29, 28, 27, 26};
    src.evs = {10, 20, 30, 40, 50, 60};
    src.moves = {84, 85, 86, 87};  // ThunderShock, Thunderbolt, ThunderWave, Thunder
    src.pp = {30, 15, 20, 10};
    src.heldItem = 0;
    src.abilitySlot = 1;
    src.metLevel = 5;
    src.ball = 4;

    std::array<uint8_t, 80> blob{};
    assert(encodeBoxMon(src, blob.data()));

    // Encrypted region should not be all zeros for a real mon.
    bool encNonZero = false;
    for (int i = 32; i < 80; ++i) {
        if (blob[i] != 0) {
            encNonZero = true;
            break;
        }
    }
    assert(encNonZero);

    Pokemon dst;
    assert(decodeBoxMon(blob.data(), dst));
    assert(dst.species == src.species);
    assert(dst.isShiny == true);
    assert(dst.moves[0] == src.moves[0]);
    assert(dst.moves[1] == src.moves[1]);
    assert(dst.moves[2] == src.moves[2]);
    assert(dst.moves[3] == src.moves[3]);
    assert(dst.ivs.hp == src.ivs.hp);
    assert(dst.ivs.atk == src.ivs.atk);
    assert(dst.ivs.def == src.ivs.def);
    assert(dst.ivs.spe == src.ivs.spe);
    assert(dst.ivs.spa == src.ivs.spa);
    assert(dst.ivs.spd == src.ivs.spd);
    assert(dst.evs.hp == src.evs.hp);
    assert(dst.evs.atk == src.evs.atk);
    assert(dst.friendship == src.friendship);
    assert(dst.otId == src.otId);
    assert(dst.heldItem == src.heldItem);
    assert(dst.abilitySlot == src.abilitySlot);

    // Empty mon round-trip
    Pokemon empty;
    std::array<uint8_t, 80> emptyBlob{};
    assert(encodeBoxMon(empty, emptyBlob.data()));
    Pokemon emptyOut;
    assert(!decodeBoxMon(emptyBlob.data(), emptyOut));
    assert(emptyOut.empty());

    // Non-shiny encode
    Pokemon plain = src;
    plain.isShiny = false;
    plain.pid = 0x11223344;
    plain.otSecretId = 9999;
    std::array<uint8_t, 80> plainBlob{};
    assert(encodeBoxMon(plain, plainBlob.data()));
    Pokemon plainOut;
    assert(decodeBoxMon(plainBlob.data(), plainOut));
    assert(plainOut.isShiny == false);
    assert(plainOut.species == 25);

    std::puts("gba_roundtrip_test OK");
    return 0;
}
