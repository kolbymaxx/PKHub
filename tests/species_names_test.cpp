#include "pkhub/core/pokemon/NatureNames.hpp"
#include "pkhub/core/pokemon/SpeciesIds.hpp"
#include "pkhub/core/pokemon/SpeciesNames.hpp"

#include <cstdio>
#include <cstring>

using namespace pkhub;

int main() {
    if (std::strcmp(speciesEnglishName(25), "Pikachu") != 0) {
        std::printf("Pikachu fail: %s\n", speciesEnglishName(25));
        return 1;
    }
    if (std::strcmp(speciesEnglishName(150), "Mewtwo") != 0) {
        std::printf("Mewtwo fail\n");
        return 1;
    }
    if (std::strcmp(speciesEnglishName(1008), "Miraidon") != 0) {
        std::printf("Miraidon fail: %s\n", speciesEnglishName(1008));
        return 1;
    }
    if (std::strcmp(speciesEnglishName(1013), "Sinistcha") != 0) {
        std::printf("Sinistcha fail: %s\n", speciesEnglishName(1013));
        return 1;
    }
    if (std::strcmp(natureEnglishName(0), "Hardy") != 0 ||
        std::strcmp(natureEnglishName(13), "Jolly") != 0) {
        std::printf("nature fail\n");
        return 1;
    }

    Pokemon sv;
    sv.species = 917;  // Dudunsparce internal
    sv.nativeGeneration = Generation::Gen9;
    if (pokemonDisplayName(sv) != "Dudunsparce") {
        std::printf("SV display fail: %s (nat=%u)\n", pokemonDisplayName(sv).c_str(),
                    unsigned(nationalDexId(sv)));
        return 1;
    }

    Pokemon nick;
    nick.species = 25;
    nick.nativeGeneration = Generation::Gen8;
    nick.nickname = "Sparky";
    if (pokemonDisplayName(nick) != "Sparky") {
        std::printf("nickname fail\n");
        return 1;
    }

    std::printf("species_names OK\n");
    return 0;
}
