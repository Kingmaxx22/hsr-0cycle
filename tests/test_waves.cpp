// Sequential (wave) slot verification: in a sequential slot only the
// first living entry is active; each later entry activates (with a Spawn
// event) once every earlier slot-mate is dead. Concurrent slots keep the
// old behavior (all entries active together). Plain asserts, zero deps.
#include "simulation/SimulationEngine.h"

#include <cstdio>
#include <limits>

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

CharacterConfig makeDps()
{
    CharacterConfig c;
    c.id = "wave_dps";
    c.name = "Wave DPS";
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

EnemyConfig makeFoe(const char* id, const char* name)
{
    EnemyConfig e;
    e.id = id;
    e.name = name;
    e.maxHp = 40000;
    e.currentHp = 40000;
    e.toughness = 100000; // never breaks: pure race
    e.level = 80;
    e.baseDef = 0.0;
    e.atk = 10.0; // harmless: acts without wiping
    e.spd = 150.0;
    e.resistance = 0.0f;
    e.slotIndex = 0;
    return e;
}

} // namespace

int main()
{
    // --- Sequential slot: B acts only after A dies ---
    {
        EncounterConfig enc;
        enc.slots[0].push_back(makeFoe("wave_a", "Wave A"));
        enc.slots[0].push_back(makeFoe("wave_b", "Wave B"));
        enc.sequential[0] = true;

        SimulationEngine engine;
        SimulationResult r = engine.runSimulation({makeDps()}, enc, 15000);

        int firstBAct = std::numeric_limits<int>::max();
        int lastAAct = -1;
        int spawnB = 0;
        int firstBDmg = std::numeric_limits<int>::max();
        int lastADmg = -1;
        for (size_t i = 0; i < r.timeline.size(); ++i)
        {
            const auto& ev = r.timeline[i];
            int idx = static_cast<int>(i);
            if (ev.actionType == "EnemyAtk" && ev.characterId == "wave_a")
                lastAAct = idx;
            if (ev.actionType == "EnemyAtk" && ev.characterId == "wave_b")
                firstBAct = std::min(firstBAct, idx);
            if (ev.actionType == "Spawn" && ev.targetEnemyId == "wave_b")
                ++spawnB;
            if (ev.targetEnemyId == "wave_a" && ev.damageDealt > 0)
                lastADmg = idx;
            if (ev.targetEnemyId == "wave_b" && ev.damageDealt > 0)
                firstBDmg = std::min(firstBDmg, idx);
        }
        CHECK(lastAAct >= 0);          // A acted while alive
        CHECK(firstBAct < std::numeric_limits<int>::max()); // B activated
        CHECK(firstBAct > lastAAct);   // ...only after A stopped (dead)
        CHECK(spawnB == 1);            // exactly one wave entry event
        for (const auto& ev : r.timeline)
        {
            if (ev.actionType == "Spawn")
            {
                CHECK(ev.damageDealt == 0);
                CHECK(ev.spChange == 0);
            }
        }
        CHECK(lastADmg >= 0 && firstBDmg < std::numeric_limits<int>::max());
        CHECK(lastADmg < firstBDmg);   // B takes damage only after A is done
        CHECK(r.success);              // both waves cleared
    }

    // --- Concurrent control: both act from the start, no spawns ---
    {
        EncounterConfig enc;
        enc.slots[0].push_back(makeFoe("wave_a", "Wave A"));
        enc.slots[0].push_back(makeFoe("wave_b", "Wave B"));
        enc.sequential[0] = false; // sequential is the default; opt out here

        SimulationEngine engine;
        SimulationResult r = engine.runSimulation({makeDps()}, enc, 15000);

        int firstBAct = std::numeric_limits<int>::max();
        int lastAAct = -1;
        int spawns = 0;
        for (size_t i = 0; i < r.timeline.size(); ++i)
        {
            const auto& ev = r.timeline[i];
            int idx = static_cast<int>(i);
            if (ev.actionType == "EnemyAtk" && ev.characterId == "wave_a")
                lastAAct = idx;
            if (ev.actionType == "EnemyAtk" && ev.characterId == "wave_b")
                firstBAct = std::min(firstBAct, idx);
            if (ev.actionType == "Spawn")
                ++spawns;
        }
        CHECK(lastAAct >= 0);
        CHECK(firstBAct < std::numeric_limits<int>::max());
        CHECK(firstBAct < lastAAct); // B acts while A still alive
        CHECK(spawns == 0);
        CHECK(r.success);
    }

    if (failures == 0)
        std::printf("test_waves: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
