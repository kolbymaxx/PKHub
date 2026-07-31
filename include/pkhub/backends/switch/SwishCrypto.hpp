#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pkhub {

enum class ScType : uint8_t {
    None = 0,
    Bool1 = 1,
    Bool2 = 2,
    Bool3 = 3,
    Object = 4,
    Array = 5,
    Byte = 8,
    UInt16 = 9,
    UInt32 = 10,
    UInt64 = 11,
    SByte = 12,
    Int16 = 13,
    Int32 = 14,
    Int64 = 15,
    Single = 16,
    Double = 17,
};

struct ScBlock {
    uint32_t key = 0;
    ScType type = ScType::None;
    ScType subType = ScType::None;
    std::vector<uint8_t> data;
};

/**
 * Clean-room SwishCrypto ("MemeCrypto V2") for Switch Pokémon saves.
 * Format facts (hash salts / xorpad / SCBlock layout) are public RE knowledge;
 * this is an original C++ implementation — not a port of any editor's source.
 */
class SwishCrypto {
public:
    static constexpr std::size_t kHashSize = 32;

    static bool isHashValid(const std::vector<uint8_t>& data);
    static bool decrypt(std::vector<uint8_t> data, std::vector<ScBlock>& out, std::string* err);
    static bool encrypt(const std::vector<ScBlock>& blocks, std::vector<uint8_t>& out, std::string* err);

    static ScBlock* findBlock(std::vector<ScBlock>& blocks, uint32_t key);
    static const ScBlock* findBlock(const std::vector<ScBlock>& blocks, uint32_t key);

    static void cryptStaticXorpad(std::vector<uint8_t>& payload);
};

int scTypeSize(ScType type);

}  // namespace pkhub
