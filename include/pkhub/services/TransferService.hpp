#pragma once

#include <string>

#include "pkhub/core/save/ISaveBackend.hpp"

namespace pkhub {

enum class TransferMode {
    Move,
    Copy,
};

struct SlotRef {
    IBoxProvider* provider = nullptr;
    bool fromParty = false;
    std::size_t boxIndex = 0;
    std::size_t slotIndex = 0;
};

struct TransferResult {
    bool ok = false;
    std::string message;
};

class TransferService {
public:
    TransferResult transfer(SlotRef src, SlotRef dst, TransferMode mode);

    /// Swap two occupied/empty slots.
    TransferResult swap(SlotRef a, SlotRef b);
};

}  // namespace pkhub
