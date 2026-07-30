#pragma once

#include <string>
#include <vector>

#include "pkhub/core/pokemon/Pokemon.hpp"

namespace pkhub {

enum class RiskLevel : uint8_t {
    None = 0,
    Info,
    Caution,   // may fail online checks
    High,      // likely rejected / ban-risk messaging
};

struct LegalityFinding {
    RiskLevel risk = RiskLevel::None;
    std::string code;     // stable machine id e.g. "IV_OUT_OF_RANGE"
    std::string message;  // user-facing
};

struct LegalityReport {
    RiskLevel overall = RiskLevel::None;
    std::vector<LegalityFinding> findings;

    bool hasRisk() const { return overall >= RiskLevel::Caution; }
};

/// Advisory checks only — never silently mutates Pokémon.
class LegalityService {
public:
    LegalityReport evaluate(const Pokemon& mon, GameId contextGame) const;
};

const char* riskLevelLabel(RiskLevel level);

}  // namespace pkhub
