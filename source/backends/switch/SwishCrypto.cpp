#include "pkhub/backends/switch/SwishCrypto.hpp"

#include "sha256.hpp"

#include <cstring>

namespace pkhub {
namespace {

constexpr uint8_t kIntroHash[64] = {
    0x9E, 0xC9, 0x9C, 0xD7, 0x0E, 0xD3, 0x3C, 0x44, 0xFB, 0x93, 0x03, 0xDC, 0xEB, 0x39, 0xB4, 0x2A,
    0x19, 0x47, 0xE9, 0x63, 0x4B, 0xA2, 0x33, 0x44, 0x16, 0xBF, 0x82, 0xA2, 0xBA, 0x63, 0x55, 0xB6,
    0x3D, 0x9D, 0xF2, 0x4B, 0x5F, 0x7B, 0x6A, 0xB2, 0x62, 0x1D, 0xC2, 0x1B, 0x68, 0xE5, 0xC8, 0xB5,
    0x3A, 0x05, 0x90, 0x00, 0xE8, 0xA8, 0x10, 0x3D, 0xE2, 0xEC, 0xF0, 0x0C, 0xB2, 0xED, 0x4F, 0x6D,
};

constexpr uint8_t kOutroHash[64] = {
    0xD6, 0xC0, 0x1C, 0x59, 0x8B, 0xC8, 0xB8, 0xCB, 0x46, 0xE1, 0x53, 0xFC, 0x82, 0x8C, 0x75, 0x75,
    0x13, 0xE0, 0x45, 0xDF, 0x32, 0x69, 0x3C, 0x75, 0xF0, 0x59, 0xF8, 0xD9, 0xA2, 0x5F, 0xB2, 0x17,
    0xE0, 0x80, 0x52, 0xDB, 0xEA, 0x89, 0x73, 0x99, 0x75, 0x79, 0xAF, 0xCB, 0x2E, 0x80, 0x07, 0xE6,
    0xF1, 0x26, 0xE0, 0x03, 0x0A, 0xE6, 0x6F, 0xF6, 0x41, 0xBF, 0x7E, 0x59, 0xC2, 0xAE, 0x55, 0xFD,
};

// 0x80 bytes; advance stride is 0x7F (last pad byte is alignment).
constexpr uint8_t kXorPad[128] = {
    0xA0, 0x92, 0xD1, 0x06, 0x07, 0xDB, 0x32, 0xA1, 0xAE, 0x01, 0xF5, 0xC5, 0x1E, 0x84, 0x4F, 0xE3,
    0x53, 0xCA, 0x37, 0xF4, 0xA7, 0xB0, 0x4D, 0xA0, 0x18, 0xB7, 0xC2, 0x97, 0xDA, 0x5F, 0x53, 0x2B,
    0x75, 0xFA, 0x48, 0x16, 0xF8, 0xD4, 0x8A, 0x6F, 0x61, 0x05, 0xF4, 0xE2, 0xFD, 0x04, 0xB5, 0xA3,
    0x0F, 0xFC, 0x44, 0x92, 0xCB, 0x32, 0xE6, 0x1B, 0xB9, 0xB1, 0x2E, 0x01, 0xB0, 0x56, 0x53, 0x36,
    0xD2, 0xD1, 0x50, 0x3D, 0xDE, 0x5B, 0x2E, 0x0E, 0x52, 0xFD, 0xDF, 0x2F, 0x7B, 0xCA, 0x63, 0x50,
    0xA4, 0x67, 0x5D, 0x23, 0x17, 0xC0, 0x52, 0xE1, 0xA6, 0x30, 0x7C, 0x2B, 0xB6, 0x70, 0x36, 0x5B,
    0x2A, 0x27, 0x69, 0x33, 0xF5, 0x63, 0x7B, 0x36, 0x3F, 0x26, 0x9B, 0xA3, 0xED, 0x7A, 0x53, 0x00,
    0xA4, 0x48, 0xB3, 0x50, 0x9E, 0x14, 0xA0, 0x52, 0xDE, 0x7E, 0x10, 0x2B, 0x1B, 0x77, 0x6E, 0x00,
};

uint32_t readU32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

void writeU32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(uint8_t(v));
    out.push_back(uint8_t(v >> 8));
    out.push_back(uint8_t(v >> 16));
    out.push_back(uint8_t(v >> 24));
}

class ScXorShift32 {
public:
    explicit ScXorShift32(uint32_t seed) : state_(initialState(seed)) {}

    uint8_t next() {
        const uint8_t result = uint8_t(state_ >> (counter_ << 3));
        if (counter_ == 3) {
            state_ = advance(state_);
            counter_ = 0;
        } else {
            ++counter_;
        }
        return result;
    }

    int next32() {
        return int(next()) | (int(next()) << 8) | (int(next()) << 16) | (int(next()) << 24);
    }

private:
    static uint32_t advance(uint32_t state) {
        state ^= state << 2;
        state ^= state >> 15;
        state ^= state << 13;
        return state;
    }

    static uint32_t initialState(uint32_t state) {
        const int pops = __builtin_popcount(state);
        for (int i = 0; i < pops; ++i) {
            state = advance(state);
        }
        return state;
    }

    int counter_ = 0;
    uint32_t state_ = 0;
};

void computeHash(const uint8_t* payload, std::size_t len, uint8_t out[32]) {
    sha256::Ctx ctx;
    sha256::update(ctx, kIntroHash, sizeof(kIntroHash));
    sha256::update(ctx, payload, len);
    sha256::update(ctx, kOutroHash, sizeof(kOutroHash));
    sha256::final(ctx, out);
}

bool readBlock(const std::vector<uint8_t>& data, std::size_t& offset, ScBlock& block, std::string* err) {
    if (offset + 5 > data.size()) {
        if (err) {
            *err = "Truncated SCBlock header";
        }
        return false;
    }
    const uint32_t key = readU32(data.data() + offset);
    offset += 4;
    ScXorShift32 xk(key);
    const auto type = ScType(data[offset++] ^ xk.next());
    block.key = key;
    block.type = type;
    block.subType = ScType::None;
    block.data.clear();

    switch (type) {
        case ScType::Bool1:
        case ScType::Bool2:
        case ScType::Bool3:
            return true;
        case ScType::Object: {
            if (offset + 4 > data.size()) {
                if (err) {
                    *err = "Truncated SCBlock object length";
                }
                return false;
            }
            const int numBytes = int(readU32(data.data() + offset)) ^ xk.next32();
            offset += 4;
            if (numBytes < 0 || offset + std::size_t(numBytes) > data.size()) {
                if (err) {
                    *err = "Invalid SCBlock object size";
                }
                return false;
            }
            block.data.assign(data.begin() + static_cast<std::ptrdiff_t>(offset),
                              data.begin() + static_cast<std::ptrdiff_t>(offset + numBytes));
            offset += std::size_t(numBytes);
            for (auto& b : block.data) {
                b = uint8_t(b ^ xk.next());
            }
            return true;
        }
        case ScType::Array: {
            if (offset + 5 > data.size()) {
                if (err) {
                    *err = "Truncated SCBlock array header";
                }
                return false;
            }
            const int entries = int(readU32(data.data() + offset)) ^ xk.next32();
            offset += 4;
            const auto sub = ScType(data[offset++] ^ xk.next());
            const int elem = scTypeSize(sub);
            if (elem <= 0 || entries < 0) {
                if (err) {
                    *err = "Invalid SCBlock array subtype/count";
                }
                return false;
            }
            const int numBytes = entries * elem;
            if (offset + std::size_t(numBytes) > data.size()) {
                if (err) {
                    *err = "Truncated SCBlock array payload";
                }
                return false;
            }
            block.subType = sub;
            block.data.assign(data.begin() + static_cast<std::ptrdiff_t>(offset),
                              data.begin() + static_cast<std::ptrdiff_t>(offset + numBytes));
            offset += std::size_t(numBytes);
            for (auto& b : block.data) {
                b = uint8_t(b ^ xk.next());
            }
            return true;
        }
        default: {
            const int numBytes = scTypeSize(type);
            if (numBytes <= 0 || offset + std::size_t(numBytes) > data.size()) {
                if (err) {
                    *err = "Invalid SCBlock value";
                }
                return false;
            }
            block.data.assign(data.begin() + static_cast<std::ptrdiff_t>(offset),
                              data.begin() + static_cast<std::ptrdiff_t>(offset + numBytes));
            offset += std::size_t(numBytes);
            for (auto& b : block.data) {
                b = uint8_t(b ^ xk.next());
            }
            return true;
        }
    }
}

void writeBlock(std::vector<uint8_t>& out, const ScBlock& block) {
    writeU32(out, block.key);
    ScXorShift32 xk(block.key);
    out.push_back(uint8_t(uint8_t(block.type) ^ xk.next()));
    if (block.type == ScType::Object) {
        writeU32(out, uint32_t(block.data.size()) ^ uint32_t(xk.next32()));
    } else if (block.type == ScType::Array) {
        const int elem = scTypeSize(block.subType);
        const int entries = elem > 0 ? int(block.data.size() / elem) : 0;
        writeU32(out, uint32_t(entries) ^ uint32_t(xk.next32()));
        out.push_back(uint8_t(uint8_t(block.subType) ^ xk.next()));
    }
    for (uint8_t b : block.data) {
        out.push_back(uint8_t(b ^ xk.next()));
    }
}

}  // namespace

int scTypeSize(ScType type) {
    switch (type) {
        case ScType::Bool3:
        case ScType::Byte:
        case ScType::SByte:
            return 1;
        case ScType::UInt16:
        case ScType::Int16:
            return 2;
        case ScType::UInt32:
        case ScType::Int32:
        case ScType::Single:
            return 4;
        case ScType::UInt64:
        case ScType::Int64:
        case ScType::Double:
            return 8;
        default:
            return -1;
    }
}

void SwishCrypto::cryptStaticXorpad(std::vector<uint8_t>& payload) {
    constexpr std::size_t kPad = sizeof(kXorPad);
    constexpr std::size_t kStride = kPad - 1;  // 0x7F
    if (payload.empty()) {
        return;
    }
    std::size_t offset = 0;
    int iterations = int((payload.size() - 1) / kStride);
    while (iterations-- > 0) {
        for (std::size_t i = 0; i < kPad && offset + i < payload.size(); ++i) {
            payload[offset + i] ^= kXorPad[i];
        }
        offset += kStride;
    }
    for (std::size_t i = 0; offset + i < payload.size(); ++i) {
        payload[offset + i] ^= kXorPad[i];
    }
}

bool SwishCrypto::isHashValid(const std::vector<uint8_t>& data) {
    if (data.size() < kHashSize) {
        return false;
    }
    uint8_t computed[32];
    computeHash(data.data(), data.size() - kHashSize, computed);
    return std::memcmp(computed, data.data() + data.size() - kHashSize, kHashSize) == 0;
}

bool SwishCrypto::decrypt(std::vector<uint8_t> data, std::vector<ScBlock>& out, std::string* err) {
    out.clear();
    if (data.size() < kHashSize) {
        if (err) {
            *err = "Save too small for SwishCrypto";
        }
        return false;
    }
    if (!isHashValid(data)) {
        if (err) {
            *err = "SwishCrypto hash mismatch (not a Switch Pokémon save, or corrupt)";
        }
        return false;
    }
    data.resize(data.size() - kHashSize);
    cryptStaticXorpad(data);

    std::size_t offset = 0;
    while (offset < data.size()) {
        ScBlock block;
        if (!readBlock(data, offset, block, err)) {
            return false;
        }
        out.push_back(std::move(block));
    }
    return true;
}

bool SwishCrypto::encrypt(const std::vector<ScBlock>& blocks, std::vector<uint8_t>& out, std::string* err) {
    (void)err;
    out.clear();
    for (const auto& b : blocks) {
        writeBlock(out, b);
    }
    cryptStaticXorpad(out);
    uint8_t hash[32];
    computeHash(out.data(), out.size(), hash);
    out.insert(out.end(), hash, hash + 32);
    return true;
}

ScBlock* SwishCrypto::findBlock(std::vector<ScBlock>& blocks, uint32_t key) {
    for (auto& b : blocks) {
        if (b.key == key) {
            return &b;
        }
    }
    return nullptr;
}

const ScBlock* SwishCrypto::findBlock(const std::vector<ScBlock>& blocks, uint32_t key) {
    for (const auto& b : blocks) {
        if (b.key == key) {
            return &b;
        }
    }
    return nullptr;
}

}  // namespace pkhub
