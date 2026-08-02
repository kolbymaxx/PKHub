#include "pkhub/core/pokemon/NatureNames.hpp"

namespace pkhub {
namespace {

// Official English nature order (PID / nature byte 0..24).
constexpr const char* kNatures[] = {
    "Hardy",   "Lonely",  "Brave",   "Adamant", "Naughty",
    "Bold",    "Docile",  "Relaxed", "Impish",  "Lax",
    "Timid",   "Hasty",   "Serious", "Jolly",   "Naive",
    "Modest",  "Mild",    "Quiet",   "Bashful", "Rash",
    "Calm",    "Gentle",  "Sassy",   "Careful", "Quirky",
};

}  // namespace

const char* natureEnglishName(int8_t nature) {
    if (nature < 0 || nature > 24) {
        return "";
    }
    return kNatures[nature];
}

}  // namespace pkhub
