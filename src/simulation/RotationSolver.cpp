#include "RotationSolver.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>

namespace hsr {

RotationSolver::RotationSolver(int maxPatternLen, int maxRounds)
    : m_maxPatternLen(std::max(1, std::min(3, maxPatternLen))),
      m_maxRounds(std::max(1, maxRounds)) {}

std::vector<std::string> RotationSolver::alphabetFor(
    const CharacterConfig& c) const {
    std::vector<std::string> alpha = {"Basic", "Skill", "Ult"};
    if (c.healMultiplier > 0.0) alpha.push_back("Heal");
    if (c.shieldMultiplier > 0.0) alpha.push_back("Shield");
    // Per-action DB tuning can also enable support actions.
    for (const auto& kv : c.skillActions) {
        if (kv.first == "Heal" &&
            std::find(alpha.begin(), alpha.end(), "Heal") == alpha.end()) {
            bool usable = kv.second.healMultiplier > 0.0;
            if (usable) alpha.push_back("Heal");
        }
        if (kv.first == "Shield" &&
            std::find(alpha.begin(), alpha.end(), "Shield") == alpha.end()) {
            bool usable = kv.second.shieldMultiplier > 0.0;
            if (usable) alpha.push_back("Shield");
        }
    }
    return alpha;
}

bool RotationSolver::isZeroCost(const std::vector<std::string>& pattern) const {
    for (const auto& a : pattern) {
        if (a != "Ult" && a != "FUA")
            return false;
    }
    return true;
}

std::vector<std::vector<std::string>> RotationSolver::allPatterns(
    const std::vector<std::string>& alphabet) const {
    std::vector<std::vector<std::string>> out;
    std::vector<std::string> current;
    // DFS over pattern lengths 1..max (order: short first, stable).
    std::function<void()> dfs = [&]() {
        if (!current.empty() && !isZeroCost(current))
            out.push_back(current);
        if (static_cast<int>(current.size()) >= m_maxPatternLen)
            return;
        for (const auto& a : alphabet) {
            current.push_back(a);
            dfs();
            current.pop_back();
        }
    };
    dfs();
    return out;
}

long long RotationSolver::scoreOf(const SimulationResult& r) const {
    // Lexicographic as documented: clear < non-clear; clears by AV;
    // non-clears by damage (negated into a minimization score).
    const long long INF = static_cast<long long>(4e12);
    if (r.timeline.empty())
        return INF;
    if (r.isZeroCycleClear || (r.success && r.totalCycles == 0)) {
        int av = r.timeline.back().currentAv;
        return static_cast<long long>(av);
    }
    if (r.success) {
        // Cleared past 0-cycle: worse than any 0-cycle, better than wipes.
        int av = r.timeline.back().currentAv;
        return INF / 4 + static_cast<long long>(av);
    }
    // Failed: higher damage is better (negated damage, offset below clears).
    return INF / 2 - static_cast<long long>(r.totalDamage);
}

SolverResult RotationSolver::solve(
    const std::vector<CharacterConfig>& characters,
    const EncounterConfig& encounter,
    int avLimit) {
    SolverResult best;
    best.rotations.resize(characters.size(), {"Basic"});
    best.evalCount = 0;

    if (characters.empty())
        return best;

    // Candidate patterns per character (rotation-independent).
    std::vector<std::vector<std::vector<std::string>>> candidates;
    for (const auto& c : characters)
        candidates.push_back(allPatterns(alphabetFor(c)));

    // Order: fastest effective speed first (acts most, decides most).
    SimulationEngine probe;
    std::vector<size_t> order(characters.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return probe.effectiveSpeed(characters[a]) >
               probe.effectiveSpeed(characters[b]);
    });

    auto evaluate = [&](const std::vector<std::vector<std::string>>& rots,
                        SimulationResult& out) -> long long {
        std::vector<CharacterConfig> configs = characters;
        for (size_t i = 0; i < configs.size(); ++i)
            configs[i].rotation = rots[i];
        SimulationEngine engine; // fresh RNG per eval: deterministic
        out = engine.runSimulation(configs, encounter, avLimit);
        ++best.evalCount;
        return scoreOf(out);
    };

    // Baseline: all-Basic patterns.
    std::vector<std::vector<std::string>> current = best.rotations;
    SimulationResult curResult;
    long long curScore = evaluate(current, curResult);

    for (int round = 0; round < m_maxRounds; ++round) {
        bool improved = false;
        for (size_t idx : order) {
            long long localBest = curScore;
            std::vector<std::string> localPattern = current[idx];
            SimulationResult localResult = curResult;
            for (const auto& pattern : candidates[idx]) {
                if (pattern == current[idx])
                    continue;
                std::vector<std::vector<std::string>> trial = current;
                trial[idx] = pattern;
                SimulationResult trialResult;
                long long trialScore = evaluate(trial, trialResult);
                if (trialScore < localBest) {
                    localBest = trialScore;
                    localPattern = pattern;
                    localResult = trialResult;
                }
            }
            if (localBest < curScore) {
                curScore = localBest;
                current[idx] = localPattern;
                curResult = localResult;
                improved = true;
            }
        }
        if (!improved)
            break;
    }

    best.rotations = current;
    best.result = curResult;
    best.totalDamage = curResult.totalDamage;
    best.cleared =
        curResult.isZeroCycleClear || (curResult.success && curResult.totalCycles == 0);
    if (best.cleared && !curResult.timeline.empty())
        best.clearAv = curResult.timeline.back().currentAv;
    return best;
}

} // namespace hsr
