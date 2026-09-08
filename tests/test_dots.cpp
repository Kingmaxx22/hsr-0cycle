// Phase 4.2 verification: break-DoT application (EHR-gated) and ticking.
// Plain asserts, zero deps. Deterministic RNG (fixed seed in engine).
#include "simulation/SimulationEngine.h"

#include <cstdio>

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

CharacterConfig makeChar()
{
    CharacterConfig c;
    c.id = "dotter";
    c.name = "dotter";
    c.manualStats = true;
    c.speed = 200;
    c.finalAtk = 3000.0;
    c.finalHp = 20000.0;
    c.finalDef = 1000.0;
    c.level = 80;
    c.element = "Fire";
    c.skillMultiplier = 1.0;
    c.rotation = {"Skill"};
    // Break DoT config: guaranteed application, 2 ticks.
    c.breakDotType = "Burn";
    c.breakDotChance = 1.0;
    c.breakDotTurns = 2;
    c.breakDotAtkScale = 1.0;
    return c;
}

EnemyConfig makeFoe()
{
    EnemyConfig e;
    e.id = "foe";
    e.name = "foe";
    e.maxHp = 500000;
    e.currentHp = 500000;
    e.toughness = 20; // one Skill (20 toughness) breaks; re-breaks re-apply
    e.level = 80;
    e.baseDef = 500.0;
    e.spd = 50.0; // slow: acts (and ticks) after the break
    e.slotIndex = 0;
    e.effectRes = 0.0;
    return e;
}

int countTicks(const SimulationResult& r, bool& applied)
{
    int dotTicks = 0;
    for (const auto& ev : r.timeline)
    {
        if (!ev.debuffApplied.empty() && ev.actionType != "DotTick")
        {
            if (ev.debuffApplied != "Burn")
            {
                std::printf("unexpected debuff %s\n", ev.debuffApplied.c_str());
                ++failures;
            }
            applied = true;
        }
        if (ev.actionType == "DotTick")
        {
            ++dotTicks;
            if (ev.damageDealt <= 0 || ev.targetEnemyId != "foe")
                ++failures;
        }
    }
    return dotTicks;
}

} // namespace

int main()
{
    // 1. Default config: no DoT behavior (debuffApplied empty, no DotTick).
    {
        SimulationEngine engine;
        CharacterConfig c = makeChar();
        c.breakDotType = "";
        c.breakDotChance = 0.0;
        EncounterConfig enc;
        enc.slots[0].push_back(makeFoe());
        SimulationResult r = engine.runSimulation({c}, enc, 12000);
        for (const auto& ev : r.timeline)
        {
            CHECK(ev.actionType != "DotTick");
            CHECK(ev.debuffApplied.empty());
        }
    }

    // 2. Break DoT applies, ticks, and expires. The foe re-breaks inside
    // the window (intended: re-breaks re-apply), so expiry is proven
    // differentially over the same AV window (enemy turns at 200, 400):
    // turns=1 -> tick, expire, re-apply, tick = 2 ticks. turns=2 ->
    // tick, re-apply, tick x2 = 3 ticks. Without expiry, turns=1 would
    // also yield 3.
    {
        SimulationEngine engine;
        CharacterConfig c = makeChar();
        c.breakDotTurns = 1;
        EncounterConfig enc;
        enc.slots[0].push_back(makeFoe());
        SimulationResult r = engine.runSimulation({c}, enc, 500);
        bool applied = false;
        CHECK(countTicks(r, applied) == 2);
        CHECK(applied);
    }
    {
        SimulationEngine engine;
        EncounterConfig enc;
        enc.slots[0].push_back(makeFoe());
        SimulationResult r = engine.runSimulation({makeChar()}, enc, 500);
        bool applied = false;
        CHECK(countTicks(r, applied) == 3);
        CHECK(applied);
    }

    // 3. Full Effect RES blocks application (chance 1.0 x (1 - 1.0) = 0).
    {
        SimulationEngine engine;
        CharacterConfig c = makeChar();
        EncounterConfig enc;
        EnemyConfig foe = makeFoe();
        foe.effectRes = 1.0;
        enc.slots[0].push_back(foe);
        SimulationResult r = engine.runSimulation({c}, enc, 12000);
        for (const auto& ev : r.timeline)
            CHECK(ev.actionType != "DotTick");
    }

    if (failures == 0)
        std::printf("test_dots: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
