#pragma once

#include "pkhub/core/safety/LegalityService.hpp"

namespace pkhub {

/**
 * Soft warnings by default; confirm only for brick / extreme risk actions.
 * Power users may still create arbitrary Pokémon after acknowledging.
 */
enum class SafetyAction : uint8_t {
    EditPokemon,          // soft banner / indicators only
    WriteSave,            // soft reminder + auto-backup
    InjectLikelyIllegal,  // soft warning (HOME/online)
    DisableBackup,        // CONFIRM — can lose recovery path
    OverwriteWithoutBackup,  // CONFIRM — high risk
    DeleteBoxWithData,    // CONFIRM — irreversible data loss
    RawHexEdit,           // CONFIRM — can brick save
};

enum class SafetyGate : uint8_t {
    Allow,           // proceed silently (or with non-blocking banner)
    SoftWarn,        // show warning, no modal required
    RequireConfirm,  // modal must be accepted
};

struct SafetyDecision {
    SafetyGate gate = SafetyGate::Allow;
    RiskLevel risk = RiskLevel::None;
    std::string title;
    std::string message;
};

class SafetyPolicy {
public:
    SafetyDecision evaluate(SafetyAction action, const LegalityReport* legality = nullptr) const;

    /// If true, even SoftWarn surfaces a one-tap dismissible tip (settings).
    bool verboseTips = true;
};

}  // namespace pkhub
