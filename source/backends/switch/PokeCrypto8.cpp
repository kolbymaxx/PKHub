#include "pkhub/backends/switch/PokeCrypto8.hpp"

#include <algorithm>
#include <cstring>

namespace pkhub::poke_crypto8 {
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

constexpr uint8_t kBlockPosition[] = {
    0, 1, 2, 3, 0, 1, 3, 2, 0, 2, 1, 3, 0, 3, 1, 2, 0, 2, 3, 1, 0, 3, 2, 1, 1, 0, 2, 3, 1, 0, 3, 2,
    2, 0, 1, 3, 3, 0, 1, 2, 2, 0, 3, 1, 3, 0, 2, 1, 1, 2, 0, 3, 1, 3, 0, 2, 2, 1, 0, 3, 3, 1, 0, 2,
    2, 3, 0, 1, 3, 2, 0, 1, 1, 2, 3, 0, 1, 3, 2, 0, 2, 1, 3, 0, 3, 1, 2, 0, 2, 3, 1, 0, 3, 2, 1, 0,
    // duplicates of 0-7
    0, 1, 2, 3, 0, 1, 3, 2, 0, 2, 1, 3, 0, 3, 1, 2, 0, 2, 3, 1, 0, 3, 2, 1, 1, 0, 2, 3, 1, 0, 3, 2,
};

constexpr uint8_t kBlockPositionInvert[] = {
    0, 1, 2, 4, 3, 5, 6, 7, 12, 18, 13, 19, 8, 10, 14, 20, 16, 22, 9, 11, 15, 21, 17, 23,
    0, 1, 2, 4, 3, 5, 6, 7,
};

void cryptArray(uint8_t* data, std::size_t len, uint32_t seed) {
    for (std::size_t i = 0; i + 1 < len; i += 2) {
        seed = 0x41C64E6Du * seed + 0x00006073u;
        const uint16_t xorv = uint16_t(seed >> 16);
        const uint16_t cur = readU16(data + i);
        writeU16(data + i, uint16_t(cur ^ xorv));
    }
}

void swapBlocks(uint8_t* data, int aWords, int bWords, int countWords) {
    for (int i = 0; i < countWords; ++i) {
        // each "word" here is 8 bytes (ulong) for gen8 shuffle
        uint8_t tmp[8];
        std::memcpy(tmp, data + (aWords + i) * 8, 8);
        std::memcpy(data + (aWords + i) * 8, data + (bWords + i) * 8, 8);
        std::memcpy(data + (bWords + i) * 8, tmp, 8);
    }
}

void shuffle8(uint8_t* data, uint32_t sv) {
    if (sv == 0) {
        return;
    }
    constexpr int sizeOfT = 8;  // ulong
    const int count = int(kBlockSize) / sizeOfT;
    uint8_t perm[kBlockCount];
    uint8_t slotOf[kBlockCount];
    for (uint8_t s = 0; s < kBlockCount; ++s) {
        perm[s] = slotOf[s] = s;
    }
    const uint8_t* shuffle = &kBlockPosition[std::size_t(sv) * kBlockCount];
    for (uint8_t i = 0; i < kBlockCount - 1; ++i) {
        const uint8_t desired = shuffle[i];
        const uint8_t j = slotOf[desired];
        if (j == i) {
            continue;
        }
        swapBlocks(data, i * count, j * count, count);
        const uint8_t blockAtI = perm[i];
        perm[j] = blockAtI;
        slotOf[blockAtI] = j;
    }
}

}  // namespace

bool isEncrypted(const uint8_t* data, std::size_t len) {
    if (len < kPartySize) {
        return false;
    }
    return readU16(data + 0x70) != 0 || readU16(data + 0x110) != 0;
}

void decrypt(uint8_t* data, std::size_t len) {
    if (len != kStoredSize && len != kPartySize) {
        return;
    }
    const uint32_t pv = readU32(data);
    const uint32_t sv = (pv >> 13) & 31;
    cryptArray(data + 8, kStoredSize - 8, pv);
    if (len > kStoredSize) {
        cryptArray(data + kStoredSize, len - kStoredSize, pv);
    }
    shuffle8(data + 8, sv);
}

void encrypt(uint8_t* data, std::size_t len) {
    if (len != kStoredSize && len != kPartySize) {
        return;
    }
    const uint32_t pv = readU32(data);
    uint32_t sv = (pv >> 13) & 31;
    sv = kBlockPositionInvert[sv];
    shuffle8(data + 8, sv);
    cryptArray(data + 8, kStoredSize - 8, pv);
    if (len > kStoredSize) {
        cryptArray(data + kStoredSize, len - kStoredSize, pv);
    }
}

void decryptIfEncrypted(std::vector<uint8_t>& data) {
    if (data.size() >= kPartySize && isEncrypted(data.data(), data.size())) {
        decrypt(data.data(), data.size());
    } else if (data.size() == kStoredSize && isEncrypted(data.data(), data.size())) {
        decrypt(data.data(), data.size());
    }
}

void encryptInPlace(std::vector<uint8_t>& data) {
    if (data.size() == kStoredSize || data.size() == kPartySize) {
        encrypt(data.data(), data.size());
    }
}

uint16_t checksumStored(const uint8_t* data) {
    uint32_t sum = 0;
    for (std::size_t i = 8; i + 1 < kStoredSize; i += 2) {
        sum += readU16(data + i);
    }
    return uint16_t(sum);
}

void refreshChecksum(uint8_t* data) {
    writeU16(data + 6, checksumStored(data));
}

}  // namespace pkhub::poke_crypto8
