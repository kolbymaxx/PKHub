#include "pkhub/core/pokemon/SpeciesIds.hpp"

#include <cstdio>

using namespace pkhub;

int main() {
    if (svInternalToNational(25) != 25) {
        std::printf("identity fail 25\n");
        return 1;
    }
    if (svInternalToNational(917) != 982) {
        std::printf("Dudunsparce fail\n");
        return 1;
    }
    if (svInternalToNational(977) != 1000) {
        std::printf("Gholdengo fail\n");
        return 1;
    }
    if (svInternalToNational(1000) != 957) {
        std::printf("Tinkatink fail\n");
        return 1;
    }
    if (svInternalToNational(1025) != 1013) {
        std::printf("Sinistcha fail\n");
        return 1;
    }

    Pokemon swsh;
    swsh.species = 25;
    swsh.nativeGeneration = Generation::Gen8;
    if (nationalDexId(swsh) != 25) {
        std::printf("swsh national fail\n");
        return 1;
    }

    Pokemon sv;
    sv.species = 917;
    sv.nativeGeneration = Generation::Gen9;
    if (nationalDexId(sv) != 982) {
        std::printf("sv national fail\n");
        return 1;
    }

    std::printf("species_ids OK\n");
    return 0;
}
