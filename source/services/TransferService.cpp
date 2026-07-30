#include "pkhub/services/TransferService.hpp"

namespace pkhub {

static Pokemon* resolveMutable(SlotRef ref) {
    if (!ref.provider) {
        return nullptr;
    }
    if (ref.fromParty) {
        if (ref.slotIndex >= ref.provider->party().size()) {
            return nullptr;
        }
        return &ref.provider->party().slot(ref.slotIndex);
    }
    if (ref.boxIndex >= ref.provider->boxCount()) {
        return nullptr;
    }
    auto& box = ref.provider->box(ref.boxIndex);
    if (ref.slotIndex >= box.size()) {
        return nullptr;
    }
    return &box.slot(ref.slotIndex);
}

TransferResult TransferService::transfer(SlotRef src, SlotRef dst, TransferMode mode) {
    Pokemon* s = resolveMutable(src);
    Pokemon* d = resolveMutable(dst);
    if (!s || !d) {
        return {false, "Invalid slot"};
    }
    if (src.provider && src.provider->isReadOnly()) {
        return {false, "Source is read-only"};
    }
    if (dst.provider && dst.provider->isReadOnly()) {
        return {false, "Destination is read-only"};
    }
    if (s->empty()) {
        return {false, "Source slot is empty"};
    }
    if (!d->empty() && mode == TransferMode::Copy) {
        return {false, "Destination occupied"};
    }
    if (!d->empty() && mode == TransferMode::Move) {
        return swap(src, dst);
    }

    *d = *s;
    if (mode == TransferMode::Move) {
        s->clear();
    }
    return {true, {}};
}

TransferResult TransferService::swap(SlotRef a, SlotRef b) {
    Pokemon* pa = resolveMutable(a);
    Pokemon* pb = resolveMutable(b);
    if (!pa || !pb) {
        return {false, "Invalid slot"};
    }
    Pokemon tmp = *pa;
    *pa = *pb;
    *pb = tmp;
    return {true, {}};
}

}  // namespace pkhub
