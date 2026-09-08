#ifndef ROTATION_SOLVER_H
#define ROTATION_SOLVER_H

#include "SimulationEngine.h"

#include <string>
#include <vector>

namespace hsr {

// Rotation solver: coordinate-descent search over per-character action
// patterns (DFS over the pattern product would be 39^4+ sims; descent
// evaluates hundreds). Objective, lexicographic:
//   1. clear the encounter (any clear beats any non-clear),
//   2. among clears, the smallest clearing AV,
//   3. among non-clears, the largest total damage.
//
// Pattern alphabet per character: Basic + Skill, plus Ult (always legal),
// plus Heal/Shield only when the character configures those multipliers.
// FUA/Memosprite are reactive, not rotational, and are never sequenced.
// All-zero-cost patterns (all Ult) are excluded: they consume no AV and
// would loop forever (the engine action cap is the backstop).
//
// Two documented limitations (not hidden):
// - Ult energy is not modeled by the engine, so a sequenced Ult always
//   fires. Patterns with Ult are optimistic until energy gating lands.
// - SP pools are per-character in the engine (not team-shared), so the
//   search treats SP as each character's own resource.
struct SolverResult {
    bool cleared = false;
    int clearAv = 0;              // timeline AV of the clearing blow (0 if none)
    float totalDamage = 0.0f;
    std::vector<std::vector<std::string>> rotations; // per character
    SimulationResult result;      // best full run (rotations applied)
    int evalCount = 0;
};

class RotationSolver {
public:
    // maxPatternLen 1..3 (3^3=27 patterns per action-triple alphabet).
    // maxRounds bounds the coordinate-descent passes.
    RotationSolver(int maxPatternLen = 3, int maxRounds = 3);

    SolverResult solve(const std::vector<CharacterConfig>& characters,
                       const EncounterConfig& encounter,
                       int avLimit = 15000);

private:
    int m_maxPatternLen;
    int m_maxRounds;

    std::vector<std::string> alphabetFor(const CharacterConfig& c) const;
    std::vector<std::vector<std::string>> allPatterns(
        const std::vector<std::string>& alphabet) const;
    bool isZeroCost(const std::vector<std::string>& pattern) const;
    // Lower is better.
    long long scoreOf(const SimulationResult& r) const;
};

} // namespace hsr

#endif // ROTATION_SOLVER_H
