#include "pkhub/core/pokemon/SpeciesIds.hpp"

namespace pkhub {
namespace {

// Internal indices 917..1025 → National Dex.
// Source: Bulbapedia "List of Pokémon by index number in Generation IX"
// joined to PokeAPI pokemon_species national ids by English name.
constexpr uint16_t kSvInternal917ToNational[] = {
    982, 917, 918, 919, 920, 953, 954, 971,  // 917-924
    972, 955, 956, 981, 960, 961, 977, 976,  // 925-932
    963, 964, 928, 929, 930, 951, 952, 938,  // 933-940
    939, 965, 966, 968, 924, 925, 974, 975,  // 941-948
    996, 997, 998, 978, 967, 921, 922, 923,  // 949-956
    940, 941, 962, 931, 973, 950, 932, 933,  // 957-964
    934, 969, 970, 944, 945, 926, 927, 942,  // 965-972
    943, 946, 947, 999, 1000, 984, 986, 1009,  // 973-980
    989, 985, 987, 988, 1005, 990, 1010, 994,  // 981-988
    992, 993, 995, 991, 1006, 1003, 1002, 1001,  // 989-996
    1004, 1007, 1008, 957, 958, 959, 935, 936,  // 997-1004
    937, 948, 949, 983, 980, 979, 1017, 1011,  // 1005-1012
    1019, 1014, 1015, 1016, 1020, 1021, 1023, 1022,  // 1013-1020
    1024, 1025, 1018, 1012, 1013,              // 1021-1025
};

static_assert(sizeof(kSvInternal917ToNational) / sizeof(kSvInternal917ToNational[0]) == 109);

}  // namespace

uint16_t svInternalToNational(uint16_t internal) {
    if (internal == 0) {
        return 0;
    }
    if (internal < 917) {
        return internal;
    }
    if (internal > 1025) {
        return internal;
    }
    return kSvInternal917ToNational[internal - 917];
}

uint16_t nationalDexId(const Pokemon& mon) {
    if (mon.empty()) {
        return 0;
    }
    if (mon.nativeGeneration == Generation::Gen9) {
        return svInternalToNational(mon.species);
    }
    return mon.species;
}

}  // namespace pkhub
