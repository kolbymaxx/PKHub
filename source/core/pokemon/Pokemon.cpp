#include "pkhub/core/pokemon/Pokemon.hpp"

namespace pkhub {

void Pokemon::clear() {
    *this = Pokemon{};
}

Generation Pokemon::originGeneration() const {
    if (nativeGeneration != Generation::Unknown) {
        return nativeGeneration;
    }
    return generationFor(originGame);
}

}  // namespace pkhub
