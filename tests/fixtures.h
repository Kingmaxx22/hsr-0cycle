#pragma once
// Shared combat fixtures for the CTest harnesses (Milestone 0.1).
// Each test keeps its own `static int failures = 0;` and main(); the
// check() helper and the builders below remove the copy-pasted
// makeDps/makeFoe blocks. All fixtures are deterministic by construction
// (expected-value crit, no EHR rolls unless a break DoT is configured).

#include "simulation/SimulationEngine.h"

#include <cstdio>
#include <string>

namespace hsrtest
{

inline bool check(bool cond, int& failures, const char* file, int line,
                  const char* expr)
{
    if (!cond)
    {
        ++failures;
        std::printf("FAIL %s:%d: %s\n", file, line, expr);
    }
    return cond;
}

#define HSRTEST_CHECK(cond) \
    ::hsrtest::check((cond), failures, __FILE__, __LINE__, #cond)

// Golden single-hit attacker: ATK 1000, Skill x2.0, level 80, no crit,
// no DMG%/PEN/buffs. Against tankyFoe the hand-computed Skill damage is
// exactly 2000 (base) x 0.5 (DEF) x 1.0 (everything else) = 1000.
inline hsr::CharacterConfig goldenDps()
{
    hsr::CharacterConfig c;
    c.id = "golden_dps";
    c.name = "Golden DPS";
    c.level = 80;
    c.element = "Fire";
    c.finalAtk = 1000.0;
    c.finalHp = 10000.0;
    c.finalDef = 500.0;
    c.speed = 200;
    c.manualStats = true;
    c.maxSp = 5;
    c.currentSp = 5;
    c.critRate = 0.0;
    c.critDmg = 1.5;
    c.elementalDmgPct = 0.0;
    c.resPen = 0.0;
    c.scalingStat = "atk";
    c.basicMultiplier = 1.0;
    c.skillMultiplier = 2.0;
    c.ultMultiplier = 4.0;
    c.rotation = {"Skill"};
    return c;
}

// Golden damage sponge: DEF 1000, level 80, forced RES 0, slow enough to
// act after a 200-SPD attacker, tough enough to never break.
inline hsr::EnemyConfig tankyFoe()
{
    hsr::EnemyConfig e;
    e.id = "tanky_foe";
    e.name = "Tanky Foe";
    e.maxHp = 1000000;
    e.currentHp = 1000000;
    e.toughness = 1000000;
    e.level = 80;
    e.baseDef = 1000.0;
    e.atk = 0.0;
    e.spd = 50.0;
    e.baseResOverride = 0.0; // exact RES 0, bypasses the auto-rule
    e.slotIndex = 0;
    return e;
}

// Heavy-hitting DPS used by wave/DoT scenarios (3500 ATK, Skill x3.0).
inline hsr::CharacterConfig strongDps(const std::string& id,
                                      const std::string& name)
{
    hsr::CharacterConfig c;
    c.id = id;
    c.name = name;
    c.level = 80;
    c.element = "Fire";
    c.finalAtk = 3500.0;
    c.finalHp = 500000.0;
    c.finalDef = 1200.0;
    c.speed = 200;
    c.manualStats = true;
    c.maxSp = 5;
    c.currentSp = 5;
    c.critRate = 0.70;
    c.critDmg = 1.50;
    c.scalingStat = "atk";
    c.basicMultiplier = 1.0;
    c.skillMultiplier = 3.0;
    c.ultMultiplier = 6.0;
    c.rotation = {"Skill", "Skill", "Skill"};
    return c;
}

// Fragile wave foe (40k HP, fast, harmless): dies in a few Skill hits,
// acts often enough to observe ordering, never breaks, never wipes.
inline hsr::EnemyConfig weakFoe(const std::string& id, const std::string& name)
{
    hsr::EnemyConfig e;
    e.id = id;
    e.name = name;
    e.maxHp = 40000;
    e.currentHp = 40000;
    e.toughness = 100000;
    e.level = 80;
    e.baseDef = 0.0;
    e.atk = 10.0;
    e.spd = 150.0;
    e.resistance = 0.0f;
    e.slotIndex = 0;
    return e;
}

inline hsr::EncounterConfig singleEncounter(const hsr::EnemyConfig& foe)
{
    hsr::EncounterConfig enc;
    enc.slots[0].push_back(foe);
    return enc;
}

} // namespace hsrtest
