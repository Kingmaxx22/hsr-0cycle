// Rotation solver verification: DFS pattern search with coordinate
// descent finds a clearing rotation where all-Basic fails; results are
// deterministic across runs. Plain asserts, zero deps.
#include "simulation/RotationSolver.h"

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
    c.id = "solver";
    c.name = "solver";
    c.manualStats = true;
    c.speed = 200;
    c.finalAtk = 3000.0;
    c.finalHp = 20000.0;
    c.finalDef = 1000.0;
    c.level = 80;
    c.element = "Fire";
    c.basicMultiplier = 1.0;
    c.skillMultiplier = 3.0;
    c.ultMultiplier = 5.0;
    c.rotation = {"Basic"};
    return c;
}

EnemyConfig makeFoe()
{
    EnemyConfig e;
    e.id = "foe";
    e.name = "foe";
    e.maxHp = 1000000;
    e.currentHp = 1000000;
    e.toughness = 100000; // never breaks: pure damage race
    e.level = 80;
    e.baseDef = 0.0;
    e.spd = 50.0;
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
    // Baseline: all-Basic must NOT clear (validates the test setup).
    {
        SimulationEngine engine;
        CharacterConfig c = makeChar();
        c.rotation = {"Basic"};
        SimulationResult r = engine.runSimulation({c}, makeEncounter(), 15000);
        CHECK(!r.isZeroCycleClear);
    }

    // Solver must find a clearing rotation using Skill.
    RotationSolver solver;
    SolverResult solved = solver.solve({makeChar()}, makeEncounter(), 15000);
    CHECK(solved.cleared);
    CHECK(solved.evalCount > 0);
    CHECK(solved.rotations.size() == 1);
    CHECK(!solved.rotations[0].empty());
    bool hasSkill = false;
    bool allUlt = true;
    for (const auto& a : solved.rotations[0])
    {
        if (a == "Skill") hasSkill = true;
        if (a != "Ult") allUlt = false;
    }
    CHECK(hasSkill);
    CHECK(!allUlt);
    CHECK(solved.clearAv > 0);
    CHECK(solved.clearAv <= 15000);

    // Determinism: same input -> same rotations.
    RotationSolver solver2;
    SolverResult solved2 = solver2.solve({makeChar()}, makeEncounter(), 15000);
    CHECK(solved2.cleared == solved.cleared);
    CHECK(solved2.rotations == solved.rotations);
    CHECK(solved2.clearAv == solved.clearAv);

    if (failures == 0)
        std::printf("test_solver: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
