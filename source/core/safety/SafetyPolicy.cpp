#include "pkhub/core/safety/SafetyPolicy.hpp"

namespace pkhub {

SafetyDecision SafetyPolicy::evaluate(SafetyAction action, const LegalityReport* legality) const {
    SafetyDecision d;

    switch (action) {
        case SafetyAction::EditPokemon:
            d.gate = SafetyGate::Allow;
            if (legality && legality->hasRisk()) {
                d.gate = SafetyGate::SoftWarn;
                d.risk = legality->overall;
                d.title = "Legality notice";
                d.message =
                    "This Pokémon may be illegal for online play or Pokémon HOME. "
                    "You can still keep the edit.";
            }
            break;

        case SafetyAction::WriteSave:
            d.gate = SafetyGate::SoftWarn;
            d.risk = RiskLevel::Info;
            d.title = "Saving";
            d.message = "A backup will be created before writing.";
            break;

        case SafetyAction::InjectLikelyIllegal:
            d.gate = SafetyGate::SoftWarn;
            d.risk = legality ? legality->overall : RiskLevel::Caution;
            d.title = "Likely illegal";
            d.message =
                "This Pokémon is likely illegal for online/HOME. "
                "It will still be written if you continue.";
            break;

        case SafetyAction::DisableBackup:
            d.gate = SafetyGate::RequireConfirm;
            d.risk = RiskLevel::High;
            d.title = "Disable backups?";
            d.message =
                "Without backups, a bad write can permanently damage your save. "
                "Only continue if you understand the risk.";
            break;

        case SafetyAction::OverwriteWithoutBackup:
            d.gate = SafetyGate::RequireConfirm;
            d.risk = RiskLevel::High;
            d.title = "Write without backup?";
            d.message = "Backup failed or was skipped. Writing now may be irreversible.";
            break;

        case SafetyAction::DeleteBoxWithData:
            d.gate = SafetyGate::RequireConfirm;
            d.risk = RiskLevel::Caution;
            d.title = "Delete box?";
            d.message = "This box still contains Pokémon. This cannot be undone.";
            break;

        case SafetyAction::RawHexEdit:
            d.gate = SafetyGate::RequireConfirm;
            d.risk = RiskLevel::High;
            d.title = "Raw edit?";
            d.message = "Raw/hex edits can brick the save. A backup is strongly recommended.";
            break;
    }

    return d;
}

}  // namespace pkhub
