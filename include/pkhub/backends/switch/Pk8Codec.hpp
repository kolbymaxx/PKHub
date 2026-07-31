#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pkhub/core/pokemon/Pokemon.hpp"

namespace pkhub {

/// Decode/encode PK8/PK9 party blobs (0x158) used by SwSh and SV boxes.
Pokemon decodePk8Party(const uint8_t* data, std::size_t len, Generation gen);
bool encodePk8Party(const Pokemon& mon, std::vector<uint8_t>& out, std::string* err);

/// Apply editable unified fields onto a decrypted native blob, then encrypt.
bool patchAndEncryptPk8(const Pokemon& mon, std::vector<uint8_t>& encryptedOut, std::string* err);

}  // namespace pkhub
