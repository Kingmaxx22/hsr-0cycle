// Golden single-hit damage test (Milestone 0.1): a fully controlled
// attacker vs a fully controlled foe must produce the hand-computed
// damage through every documented formula stage (AGENTS.md Sec 10).
//
// Hand computation (ATK 1000, Skill x2.0, L80 vs DEF 1000, L80, RES 0,
// unbroken):
//   Base  = 1000 x 2.0 = 2000
//   DMG%  = 1.0 + 0 = 1.0
//   Crit  = 1.0 + 0.0 x 1.5 = 1.0 (expected-value convention)
//   DEF   = 1 - 1000 / (1000 + 200 + 10 x 80) = 1 - 0.5 = 0.5
//   RES   = 1 - 0 = 1.0
//   Taken / Weakness / Vuln = 1.0 (all zero inputs)
//   Universal = 0.9 (built-in unbroken-toughness reduction)
//   Final = lround(2000 x 1.0 x 0.5 x 1.0 x 1.0 x 0.9) = 900
#include "fixtures.h"

#include <cmath>

using namespace hsr;

static int failures = 0;

int main()
{
    SimulationEngine engine;
    // SPD 200 acts at AV 50; the SPD-50 foe would act at AV 200, so an
    // AV limit of 50 isolates exactly one attacker action.
    SimulationResult r =
        engine.runSimulation({hsrtest::goldenDps()},
                             hsrtest::singleEncounter(hsrtest::tankyFoe()),
                             50);

    HSRTEST_CHECK(r.timeline.size() == 1);
    if (r.timeline.empty())
    {
        std::printf("test_damage: no timeline, aborting\n");
        return 1;
    }
    const ActionEvent& ev = r.timeline[0];
    HSRTEST_CHECK(ev.actionType == "Skill");
    HSRTEST_CHECK(ev.damageDealt == 900);

    // Per-stage breakdown (opaque single expressions are not accepted).
    HSRTEST_CHECK(ev.damageBreakdown.baseDamage == 2000.0);
    HSRTEST_CHECK(ev.damageBreakdown.dmgPercentMultiplier == 1.0);
    HSRTEST_CHECK(ev.damageBreakdown.defenseMultiplier == 0.5);
    HSRTEST_CHECK(ev.damageBreakdown.resistanceMultiplier == 1.0);
    HSRTEST_CHECK(ev.damageBreakdown.damageTakenMultiplier == 1.0);
    HSRTEST_CHECK(ev.damageBreakdown.universalReductionMultiplier == 0.9);
    HSRTEST_CHECK(ev.damageBreakdown.vulnerabilityMultiplier == 1.0);
    HSRTEST_CHECK(ev.critMultiplier == 1.0);
    HSRTEST_CHECK(ev.stackMultiplier == 1.0);

    if (failures == 0)
        std::printf("test_damage: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
