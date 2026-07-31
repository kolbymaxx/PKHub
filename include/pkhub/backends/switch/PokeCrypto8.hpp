#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace pkhub {

/// Gen 8/9 (SwSh/SV) Pokémon entity crypto — PK8/PK9 party size 0x158.
namespace poke_crypto8 {

constexpr std::size_t kStoredSize = 0x148;
constexpr std::size_t kPartySize = 0x158;
constexpr std::size_t kBlockSize = 0x50;
constexpr int kBlockCount = 4;

bool isEncrypted(const uint8_t* data, std::size_t len);
void decrypt(uint8_t* data, std::size_t len);
void encrypt(uint8_t* data, std::size_t len);
void decryptIfEncrypted(std::vector<uint8_t>& data);
void encryptInPlace(std::vector<uint8_t>& data);

uint16_t checksumStored(const uint8_t* data);  // bytes [8..0x148)
void refreshChecksum(uint8_t* data);

}  // namespace poke_crypto8

}  // namespace pkhub
