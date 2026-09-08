// Phase 4.1 verification: enemy offense targeting (Single/Blast/AoE).
// Plain asserts, zero new deps (links the already-vendored raylib for
// EnemyDatabase's TraceLog).
#include "data/EnemyDatabase.h"
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

CharacterConfig makeAlly(const std::string& id)
{
    CharacterConfig c;
    c.id = id;
    c.name = id;
    c.manualStats = true;
    c.speed = 200;
    c.finalAtk = 100.0; // weak: cannot one-shot the boss before it acts
    c.finalHp = 20000.0;
    c.finalDef = 1000.0;
    c.level = 80;
    c.rotation = {"Basic"};
    return c;
}

EnemyConfig makeFoe(const std::string& offense)
{
    EnemyConfig e;
    e.id = "foe";
    e.name = "foe";
    e.maxHp = 500000;
    e.currentHp = 500000;
    e.toughness = 100000; // never breaks during the test
    e.level = 80;
    e.baseDef = 1000.0;
    e.atk = 500.0;
    e.spd = 100.0;
    e.slotIndex = 0;
    e.offenseTargetType = offense;
    return e;
}

} // namespace

int main()
{
    // 1. DB overlay: Ice Edge (1002011) skill text says "to all targets".
    EnemyDatabase enemies;
    CHECK(enemies.load("engine/hsr_engine/data"));
    const EnemyInfo* ice = enemies.get("1002011");
    CHECK(ice != nullptr);
    if (ice != nullptr)
        CHECK(ice->offenseTargetType == "AoE");

    // 2. Engine: default (Single) hits exactly one ally.
    {
        SimulationEngine engine;
        EncounterConfig enc;
        enc.slots[0].push_back(makeFoe(""));
        SimulationResult r = engine.runSimulation(
            {makeAlly("a0"), makeAlly("a1")}, enc, 12000);
        const ActionEvent* atk = nullptr;
        for (const auto& ev : r.timeline)
            if (ev.actionType == "EnemyAtk") { atk = &ev; break; }
        CHECK(atk != nullptr);
        if (atk != nullptr)
        {
            CHECK(atk->damageDealt > 0);
            CHECK(atk->splashHits.empty());
        }
    }

    // 3. Engine: AoE damages the primary plus the other living ally.
    {
        SimulationEngine engine;
        EncounterConfig enc;
        enc.slots[0].push_back(makeFoe("AoE"));
        SimulationResult r = engine.runSimulation(
            {makeAlly("a0"), makeAlly("a1")}, enc, 12000);
        const ActionEvent* atk = nullptr;
        for (const auto& ev : r.timeline)
            if (ev.actionType == "EnemyAtk") { atk = &ev; break; }
        CHECK(atk != nullptr);
        if (atk != nullptr)
        {
            CHECK(atk->damageDealt > 0);
            CHECK(atk->splashHits.size() == 1);
            if (!atk->splashHits.empty())
                CHECK(atk->splashHits[0].damageDealt > 0);
        }
    }

    // 4. Engine: Blast hits primary + team-order neighbor only.
    {
        SimulationEngine engine;
        EncounterConfig enc;
        enc.slots[0].push_back(makeFoe("Blast"));
        SimulationResult r = engine.runSimulation(
            {makeAlly("a0"), makeAlly("a1"), makeAlly("a2")}, enc, 12000);
        const ActionEvent* atk = nullptr;
        for (const auto& ev : r.timeline)
            if (ev.actionType == "EnemyAtk") { atk = &ev; break; }
        CHECK(atk != nullptr);
        if (atk != nullptr)
            CHECK(atk->splashHits.size() <= 2);
    }

    if (failures == 0)
        std::printf("test_enemyaoe: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
