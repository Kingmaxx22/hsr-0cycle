// Phase 4.3 verification: multi-layered toughness and Exo-Toughness,
// driven by direct config (no data source carries these values).
// Plain asserts, zero deps.
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
    c.id = "hitter";
    c.name = "hitter";
    c.manualStats = true;
    c.speed = 200;
    c.finalAtk = 3000.0;
    c.finalHp = 20000.0;
    c.finalDef = 1000.0;
    c.level = 80;
    c.element = "Fire";
    c.skillMultiplier = 1.0;
    c.rotation = {"Skill"};
    return c;
}

EnemyConfig makeFoe()
{
    EnemyConfig e;
    e.id = "foe";
    e.name = "foe";
    e.maxHp = 500000;
    e.currentHp = 500000;
    e.toughness = 20;
    e.level = 80;
    e.baseDef = 500.0;
    e.spd = 50.0;
    e.slotIndex = 0;
    return e;
}

} // namespace

int main()
{
    // 1. Non-final layer: Break DMG is dealt but the enemy is NOT broken —
    // its next action is an attack, not a recovery.
    {
        SimulationEngine engine;
        EncounterConfig enc;
        EnemyConfig foe = makeFoe();
        foe.toughnessBars = {20, 100000};
        enc.slots[0].push_back(foe);
        SimulationResult r = engine.runSimulation({makeChar()}, enc, 500);
        bool layerBreak = false;
        for (const auto& ev : r.timeline)
        {
            if (ev.actionType == "Skill" && ev.breakDamage > 0.0f)
                layerBreak = true;
        }
        CHECK(layerBreak);
        bool sawRecover = false;
        bool sawAttack = false;
        for (const auto& ev : r.timeline)
        {
            if (ev.actionType == "EnemyRecover")
                sawRecover = true;
            if (ev.actionType == "EnemyAtk")
                sawAttack = true;
        }
        CHECK(sawAttack);
        CHECK(!sawRecover);
    }

    // 2. Exo-Toughness: hits after the break deplete it, and depleting it
    // fires a second full break event (>= 2 damage events with breakDamage).
    {
        SimulationEngine engine;
        EncounterConfig enc;
        EnemyConfig foe = makeFoe();
        foe.exoToughness = 30;
        enc.slots[0].push_back(foe);
        SimulationResult r = engine.runSimulation({makeChar()}, enc, 2000);
        int breakHits = 0;
        for (const auto& ev : r.timeline)
        {
            if ((ev.actionType == "Skill" || ev.actionType == "Basic") &&
                ev.breakDamage > 0.0f)
                ++breakHits;
        }
        CHECK(breakHits >= 2);
    }

    // 3. Defaults unchanged: no bars, no Exo — single break, then recovery.
    {
        SimulationEngine engine;
        EncounterConfig enc;
        enc.slots[0].push_back(makeFoe());
        SimulationResult r = engine.runSimulation({makeChar()}, enc, 500);
        bool sawRecover = false;
        for (const auto& ev : r.timeline)
        {
            if (ev.actionType == "EnemyRecover")
                sawRecover = true;
        }
        CHECK(sawRecover);
    }

    if (failures == 0)
        std::printf("test_bars: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
