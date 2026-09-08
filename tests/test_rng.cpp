// Deterministic RNG verification (Milestone 0.2): the same engine +
// seed replays bit-identically (fresh engines agree, repeat runs agree),
// the seed is echoed in the result, and no-RNG analysis mode applies a
// near-zero-chance DoT that rolled RNG would (almost surely) resist.
#include "fixtures.h"

using namespace hsr;

static int failures = 0;

namespace
{

bool sameTimeline(const SimulationResult& a, const SimulationResult& b)
{
    if (a.timeline.size() != b.timeline.size())
        return false;
    for (size_t i = 0; i < a.timeline.size(); ++i)
    {
        const ActionEvent& x = a.timeline[i];
        const ActionEvent& y = b.timeline[i];
        if (x.actionType != y.actionType ||
            x.characterId != y.characterId ||
            x.targetEnemyId != y.targetEnemyId ||
            x.damageDealt != y.damageDealt ||
            x.currentAv != y.currentAv ||
            x.debuffApplied != y.debuffApplied ||
            x.spChange != y.spChange)
            return false;
    }
    return a.totalDamage == b.totalDamage &&
           a.totalActions == b.totalActions;
}

bool dotApplied(const SimulationResult& r)
{
    for (const auto& ev : r.timeline)
    {
        if (!ev.debuffApplied.empty() && ev.actionType != "DotTick")
            return true;
    }
    return false;
}

} // namespace

int main()
{
    // 1. Same seed, fresh engines: identical replays (rolls matter here:
    // chance 0.5 with RES 0 decides each break application by RNG).
    {
        SimulationEngine first;
        first.setSeed(1234);
        SimulationResult r1 = first.runSimulation(
            {hsrtest::dotBreaker(0.5)},
            hsrtest::singleEncounter(hsrtest::thinFoe()), 15000);

        SimulationEngine second;
        second.setSeed(1234);
        SimulationResult r2 = second.runSimulation(
            {hsrtest::dotBreaker(0.5)},
            hsrtest::singleEncounter(hsrtest::thinFoe()), 15000);

        HSRTEST_CHECK(sameTimeline(r1, r2));
        HSRTEST_CHECK(r1.rngSeed == 1234);
        HSRTEST_CHECK(r2.rngSeed == 1234);
    }

    // 2. Same engine, run twice: reseed-per-run keeps them identical.
    {
        SimulationEngine engine;
        engine.setSeed(777);
        SimulationResult r1 = engine.runSimulation(
            {hsrtest::dotBreaker(0.5)},
            hsrtest::singleEncounter(hsrtest::thinFoe()), 15000);
        SimulationResult r2 = engine.runSimulation(
            {hsrtest::dotBreaker(0.5)},
            hsrtest::singleEncounter(hsrtest::thinFoe()), 15000);
        HSRTEST_CHECK(sameTimeline(r1, r2));
    }

    // 3. Default seed echo.
    {
        SimulationEngine engine;
        SimulationResult r = engine.runSimulation(
            {hsrtest::goldenDps()},
            hsrtest::singleEncounter(hsrtest::tankyFoe()), 50);
        HSRTEST_CHECK(engine.seed() == 42);
        HSRTEST_CHECK(r.rngSeed == 42);
    }

    // 4. No-RNG mode: a 1% base chance applies (fixed roll 0.0 < 0.01).
    {
        SimulationEngine engine;
        engine.setNoRng(true);
        SimulationResult r = engine.runSimulation(
            {hsrtest::dotBreaker(0.01)},
            hsrtest::singleEncounter(hsrtest::thinFoe()), 15000);
        HSRTEST_CHECK(dotApplied(r));
    }

    if (failures == 0)
        std::printf("test_rng: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
