#include "pkhub/core/save/ISaveBackend.hpp"

#include <stdexcept>

namespace pkhub {

Box::Box(std::size_t capacity) : slots_(capacity) {}

Pokemon& Box::slot(std::size_t index) {
    return slots_.at(index);
}

const Pokemon& Box::slot(std::size_t index) const {
    return slots_.at(index);
}

void Box::setName(std::string name) {
    name_ = std::move(name);
}

std::size_t Box::occupiedCount() const {
    std::size_t n = 0;
    for (const auto& p : slots_) {
        if (!p.empty()) {
            ++n;
        }
    }
    return n;
}

Party::Party() : slots_(kPartySlots) {}

Pokemon& Party::slot(std::size_t index) {
    return slots_.at(index);
}

const Pokemon& Party::slot(std::size_t index) const {
    return slots_.at(index);
}

std::size_t Party::occupiedCount() const {
    std::size_t n = 0;
    for (const auto& p : slots_) {
        if (!p.empty()) {
            ++n;
        }
    }
    return n;
}

}  // namespace pkhub
