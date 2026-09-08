// Phase 2 verification: Blast/AoE/Bounce splash + single-target preservation.
// Plain asserts, zero deps. Deterministic (slot-order bounce).
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
    c.id = "tester";
    c.name = "Tester";
    c.manualStats = true;
    c.speed = 100;
    c.finalAtk = 2000.0;
    c.finalHp = 5000.0;
    c.finalDef = 1000.0;
    c.level = 80;
    c.element = "Lightning";
    c.skillMultiplier = 2.0;
    c.rotation = {"Skill"};
    return c;
}

EnemyConfig makeEnemy(const std::string& id, int slot)
{
    EnemyConfig e;
    e.id = id;
    e.name = id;
    e.maxHp = 100000;
    e.currentHp = 100000;
    e.toughness = 100;
    e.level = 80;
    e.baseDef = 1000.0;
    e.spd = 50.0;
    e.slotIndex = slot;
    return e;
}

EncounterConfig threeSlots()
{
    EncounterConfig enc;
    enc.slots[0].push_back(makeEnemy("e0", 0));
    enc.slots[1].push_back(makeEnemy("e1", 1));
    enc.slots[2].push_back(makeEnemy("e2", 2));
    return enc;
}

} // namespace

int main()
{
    SimulationEngine engine;

    // 1. No tuning: single-target behavior preserved, no splash.
    {
        auto c = makeChar();
        SimulationResult r = engine.runSimulation({c}, threeSlots(), 200);
        CHECK(r.success || r.totalDamage > 0.0f);
        CHECK(r.timeline.back().splashHits.empty());
    }

    // 2. Blast: primary + slot-neighbors only (slot 0 primary -> slot 1
    // splash, slot 2 untouched).
    {
        auto c = makeChar();
        CharacterConfig::SkillActionTuning tuning;
        tuning.targetType = "Blast";
        tuning.toughnessDamage = 10;
        tuning.toughnessAdjacent = 5;
        c.skillActions["Skill"] = tuning;
        SimulationResult r = engine.runSimulation({c}, threeSlots(), 200);
        CHECK(!r.timeline.empty());
        const ActionEvent* dmg = nullptr;
        for (const auto& ev : r.timeline)
            if (ev.actionType == "Skill" && ev.damageDealt > 0) { dmg = &ev; break; }
        CHECK(dmg != nullptr);
        if (dmg)
        {
            CHECK(dmg->targetEnemyId == "e0");
            CHECK(dmg->splashHits.size() == 1);
            if (!dmg->splashHits.empty())
            {
                CHECK(dmg->splashHits[0].enemyId == "e1");
                CHECK(dmg->splashHits[0].damageDealt > 0);
            }
        }
    }

    // 3. AoE: all three enemies hit.
    {
        auto c = makeChar();
        CharacterConfig::SkillActionTuning tuning;
        tuning.targetType = "AoE";
        tuning.toughnessDamage = 10;
        c.skillActions["Skill"] = tuning;
        SimulationResult r = engine.runSimulation({c}, threeSlots(), 200);
        const ActionEvent* dmg = nullptr;
        for (const auto& ev : r.timeline)
            if (ev.actionType == "Skill" && ev.damageDealt > 0) { dmg = &ev; break; }
        CHECK(dmg != nullptr);
        if (dmg)
        {
            CHECK(dmg->splashHits.size() == 2);
            float total = static_cast<float>(dmg->damageDealt);
            for (const auto& s : dmg->splashHits)
                total += static_cast<float>(s.damageDealt);
            CHECK(r.totalDamage >= total);
        }
    }

    // 4. Bounce with bounceHits=1: primary + exactly one extra.
    {
        auto c = makeChar();
        CharacterConfig::SkillActionTuning tuning;
        tuning.targetType = "Bounce";
        tuning.toughnessDamage = 10;
        tuning.bounceHits = 1;
        c.skillActions["Skill"] = tuning;
        SimulationResult r = engine.runSimulation({c}, threeSlots(), 200);
        const ActionEvent* dmg = nullptr;
        for (const auto& ev : r.timeline)
            if (ev.actionType == "Skill" && ev.damageDealt > 0) { dmg = &ev; break; }
        CHECK(dmg != nullptr);
        if (dmg)
            CHECK(dmg->splashHits.size() == 1);
    }

    if (failures == 0)
        std::printf("test_multitarget: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
