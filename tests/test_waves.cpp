// Sequential (wave) slot verification: in a sequential slot only the
// first living entry is active; each later entry activates (with a Spawn
// event) once every earlier slot-mate is dead. Concurrent slots keep the
// old behavior (all entries active together). Uses shared fixtures.
// Plain asserts, zero deps.
#include "fixtures.h"

#include <limits>

using namespace hsr;

static int failures = 0;

int main()
{
    // --- Sequential slot: B acts only after A dies ---
    {
        EncounterConfig enc;
        enc.slots[0].push_back(hsrtest::weakFoe("wave_a", "Wave A"));
        enc.slots[0].push_back(hsrtest::weakFoe("wave_b", "Wave B"));
        enc.sequential[0] = true;

        SimulationEngine engine;
        SimulationResult r = engine.runSimulation({hsrtest::strongDps("wave_dps", "Wave DPS")}, enc, 15000);

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
        HSRTEST_CHECK(lastAAct >= 0);          // A acted while alive
        HSRTEST_CHECK(firstBAct < std::numeric_limits<int>::max()); // B activated
        HSRTEST_CHECK(firstBAct > lastAAct);   // ...only after A stopped (dead)
        HSRTEST_CHECK(spawnB == 1);            // exactly one wave entry event
        for (const auto& ev : r.timeline)
        {
            if (ev.actionType == "Spawn")
            {
                HSRTEST_CHECK(ev.damageDealt == 0);
                HSRTEST_CHECK(ev.spChange == 0);
            }
        }
        HSRTEST_CHECK(lastADmg >= 0 && firstBDmg < std::numeric_limits<int>::max());
        HSRTEST_CHECK(lastADmg < firstBDmg);   // B takes damage only after A is done
        HSRTEST_CHECK(r.success);              // both waves cleared
    }

    // --- Concurrent control: both act from the start, no spawns ---
    {
        EncounterConfig enc;
        enc.slots[0].push_back(hsrtest::weakFoe("wave_a", "Wave A"));
        enc.slots[0].push_back(hsrtest::weakFoe("wave_b", "Wave B"));
        enc.sequential[0] = false; // sequential is the default; opt out here

        SimulationEngine engine;
        SimulationResult r = engine.runSimulation({hsrtest::strongDps("wave_dps", "Wave DPS")}, enc, 15000);

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
        HSRTEST_CHECK(lastAAct >= 0);
        HSRTEST_CHECK(firstBAct < std::numeric_limits<int>::max());
        HSRTEST_CHECK(firstBAct < lastAAct); // B acts while A still alive
        HSRTEST_CHECK(spawns == 0);
        HSRTEST_CHECK(r.success);
    }

    if (failures == 0)
        std::printf("test_waves: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
