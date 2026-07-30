#pragma once

#include <cstdint>

namespace pkhub::hub_format {

constexpr uint32_t kPkboxMagic = 0x58424B50;  // 'PKBX' LE
constexpr uint16_t kBinaryVersion = 1;
constexpr int kJsonFormatVersion = 1;

constexpr const char* kHubJsonName = "hub.json";
constexpr const char* kDataDirName = "data/boxes";

}  // namespace pkhub::hub_format
