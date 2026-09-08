// End-to-end pipeline test: full data → resolver → engine → result.
// Verifies a trivial one-character/one-enemy scenario and structural
// invariants that must hold for any simulation output.  Plain asserts,
// zero extra deps.
#include "simulation/SimulationEngine.h"
#include "simulation/DamageCalculator.h"

#include <cstdio>
#include <cmath>

using namespace hsr;

static int failures = 0;
#define CHECK(cond) do { \
    if (!(cond)) { \
        ++failures; \
        std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

namespace
{

// Minimal DPS character: high ATK, high skill multiplier, fire element.
CharacterConfig makeDps()
{
    CharacterConfig c;
    c.id = "e2e_dps";
    c.name = "E2E DPS";
    c.level = 80;
    c.element = "Fire";
    c.baseHp = 1000.0;
    c.baseAtk = 800.0;
    c.baseDef = 500.0;
    c.baseSpd = 120.0;
    c.finalAtk = 3500.0;
    c.finalHp = 8000.0;
    c.finalDef = 1200.0;
    c.speed = 120;
    c.manualStats = true;
    c.maxSp = 5;
    c.currentSp = 5;
    c.critRate = 0.70;
    c.critDmg = 1.50;
    c.elementalDmgPct = 0.50;
    c.resPen = 0.10;
    c.scalingStat = "atk";
    c.basicMultiplier = 1.0;
    c.skillMultiplier = 3.0;
    c.ultMultiplier = 6.0;
    c.fuaMultiplier = 2.0;
    c.rotation = {"Skill", "Skill", "Skill"};
    return c;
}

EnemyConfig makeFoe()
{
    EnemyConfig e;
    e.id = "e2e_foe";
    e.name = "E2E Foe";
    e.maxHp = 200000;
    e.currentHp = 200000;
    e.toughness = 0;  // no break to simplify
    e.level = 80;
    e.baseDef = 1000.0;
    e.spd = 100.0;
    e.resistance = 0.0f;
    e.slotIndex = 0;
    return e;
}

EncounterConfig makeEncounter()
{
    EncounterConfig enc;
    enc.slots[0].push_back(makeFoe());
    return enc;
}

} // namespace

int main()
{
    // --- Trivial golden scenario: one DPS vs one foe, 500 AV limit ---
    {
        SimulationEngine engine;
        SimulationResult r = engine.runSimulation({makeDps()}, makeEncounter(), 500);

        // Invariant: run did not hang (action cap exists).
        CHECK(r.totalActions > 0);
        CHECK(r.totalActions <= 10000);

        // Invariant: timeline is non-empty.
        CHECK(!r.timeline.empty());

        // Count character vs enemy events.
        int charEvents = 0, enemyEvents = 0;
        for (const auto& ev : r.timeline) {
            if (ev.actionType == "EnemyAtk" || ev.actionType == "EnemyRecover" ||
                ev.actionType == "DoT")
                ++enemyEvents;
            else
                ++charEvents;
        }
        // At least one character event must exist.
        CHECK(charEvents > 0);

        // Invariant: damage is strictly positive on character actions.
        for (const auto& ev : r.timeline) {
            if (ev.actionType == "EnemyAtk" || ev.actionType == "EnemyRecover")
                continue;
            // DoT ticks may deal 0 if target is immune.
            if (ev.actionType == "DoT")
                continue;
            CHECK(ev.damageDealt > 0);
        }

        // Invariant: AV is monotonically non-decreasing.
        for (size_t i = 1; i < r.timeline.size(); ++i)
            CHECK(r.timeline[i].currentAv >= r.timeline[i - 1].currentAv);

        // Invariant: no enemy acts (foe AV > 500 limit at 100 SPD).
        for (const auto& ev : r.timeline)
            CHECK(ev.actionType != "EnemyAtk");

        // Invariant: total damage equals sum of character-action damage.
        float sum = 0.0f;
        for (const auto& ev : r.timeline) {
            if (ev.actionType != "EnemyAtk" && ev.actionType != "EnemyRecover")
                sum += static_cast<float>(ev.damageDealt);
        }
        CHECK(std::abs(r.totalDamage - sum) < 1.0f);

        // Invariant: cycles >= 0 and AV used <= limit.
        CHECK(r.totalCycles >= 0);
        if (!r.timeline.empty())
            CHECK(r.timeline.back().currentAv <= 500);
    }

    // --- Longer run: verify 0-cycle clearing behavior ---
    {
        SimulationEngine engine;
        SimulationResult r = engine.runSimulation({makeDps()}, makeEncounter(), 15000);

        // With 3500 ATK, skill=3x, 70% crit, the DPS should do >0 total.
        CHECK(r.totalDamage > 0.0f);

        // Timeline must not exceed the AV limit.
        if (!r.timeline.empty())
            CHECK(r.timeline.back().currentAv <= 15000);
    }

    // --- Two-character team: verify interleaving ---
    {
        CharacterConfig c2 = makeDps();
        c2.id = "e2e_dps2";
        c2.name = "E2E DPS 2";
        c2.speed = 150; // faster, should interleave

        SimulationEngine engine;
        SimulationResult r = engine.runSimulation({makeDps(), c2}, makeEncounter(), 500);

        CHECK(r.totalActions > 0);
        CHECK(!r.timeline.empty());

        // Both characters should act.
        bool hasC1 = false, hasC2 = false;
        for (const auto& ev : r.timeline) {
            if (ev.characterId == "e2e_dps") hasC1 = true;
            if (ev.characterId == "e2e_dps2") hasC2 = true;
        }
        CHECK(hasC1);
        CHECK(hasC2);

        // AV still monotonic (all events, including enemy).
        for (size_t i = 1; i < r.timeline.size(); ++i)
            CHECK(r.timeline[i].currentAv >= r.timeline[i - 1].currentAv);
    }

    // --- Action cap enforcement ---
    {
        SimulationEngine engine;
        // 1 action should trigger the cap.
        SimulationResult r = engine.runSimulation({makeDps()}, makeEncounter(), 15000, 1);
        CHECK(!r.errorMessage.empty());
        CHECK(r.totalActions <= 1);
    }

    if (failures == 0)
        std::printf("test_e2e: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
