#include "pkhub/core/safety/LegalityService.hpp"

namespace pkhub {

const char* riskLevelLabel(RiskLevel level) {
    switch (level) {
        case RiskLevel::None: return "OK";
        case RiskLevel::Info: return "Info";
        case RiskLevel::Caution: return "Caution";
        case RiskLevel::High: return "High risk";
        default: return "Unknown";
    }
}

LegalityReport LegalityService::evaluate(const Pokemon& mon, GameId contextGame) const {
    LegalityReport report;
    if (mon.empty()) {
        return report;
    }

    (void)contextGame;

    // Minimal Phase-0 heuristics — expand per generation later.
    const auto sumEvs = static_cast<int>(mon.evs.hp) + mon.evs.atk + mon.evs.def +
                        mon.evs.spa + mon.evs.spd + mon.evs.spe;
    if (sumEvs > 510) {
        report.findings.push_back({RiskLevel::Caution, "EV_TOTAL",
                                   "EV total exceeds 510"});
    }

    auto checkIv = [&](uint8_t v, const char* name) {
        if (v > 31) {
            report.findings.push_back({RiskLevel::Caution, "IV_RANGE",
                                       std::string(name) + " IV out of range"});
        }
    };
    checkIv(mon.ivs.hp, "HP");
    checkIv(mon.ivs.atk, "Atk");
    checkIv(mon.ivs.def, "Def");
    checkIv(mon.ivs.spa, "SpA");
    checkIv(mon.ivs.spd, "SpD");
    checkIv(mon.ivs.spe, "Spe");

    for (const auto& f : report.findings) {
        if (f.risk > report.overall) {
            report.overall = f.risk;
        }
    }
    return report;
}

}  // namespace pkhub
