// Shared team Skill Point pool verification (Milestone 1.4).
//
// SP is ONE pool for the whole team, not per-character:
//   - Any ally's Skill/Heal/Shield costs 1 from the shared pool.
//   - Any ally's Basic adds 1 to the shared pool (capped at team max).
//   - A cost is gated by the SHARED pool, so a character with plenty of
//     personal SP can still be blocked when another ally spent it.
//
// Assertions are at the EVENT level (each action's recorded spChange),
// which is exact and independent of how many turns the sim runs.
// Plain asserts, zero deps.
#include "fixtures.h"

#include <cstdio>
#include <string>

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
// Two identical DPS characters. Both want to Skill; only the pool matters.
CharacterConfig makeMember(const std::string& id, const std::string& name,
                           const std::vector<std::string>& rotation)
{
    hsr::CharacterConfig c = hsrtest::strongDps(id, name);
    c.rotation = rotation;
    return c;
}

// A fragile foe that dies to exactly two Skill hits from strongDps
// (22,680 expected damage each; 20,000 HP). With baseDef 0 the DEF
// multiplier is 1.0, so the math is exact and deterministic.
hsr::EnemyConfig fragileFoe()
{
    hsr::EnemyConfig e = hsrtest::weakFoe("fragile", "Fragile");
    e.maxHp = 20000;
    e.currentHp = 20000;
    e.baseDef = 0.0;
    e.spd = 100.0; // slower than both members, so it never acts first
    return e;
}

// Sum the recorded spChange across the timeline (a proxy for the net
// pool delta from the configured start), and count actions by type.
struct TimelineStats {
    int netSp = 0;
    int skills = 0;
    int basics = 0;
    int ults = 0;
    int heals = 0;
    int shields = 0;
    void observe(const ActionEvent& ev)
    {
        netSp += ev.spChange;
        if (ev.actionType == "Skill") ++skills;
        else if (ev.actionType == "Basic") ++basics;
        else if (ev.actionType == "Ult") ++ults;
        else if (ev.actionType == "Heal") ++heals;
        else if (ev.actionType == "Shield") ++shields;
    }
};

TimelineStats analyze(const SimulationResult& r)
{
    TimelineStats s;
    for (const auto& ev : r.timeline)
        s.observe(ev);
    return s;
}

} // namespace

int main()
{
    // 1. Shared pool drains across characters. Start the pool at 2 and give
    //    two members Skill rotations; B is faster so it acts first. Both
    //    Skills fire (pool 2 -> 1 -> 0), the fragile foe dies, and every
    //    member's finalStats reports the same shared value (0).
    {
        hsr::CharacterConfig b = makeMember("b", "B", {"Skill"});
        hsr::CharacterConfig a = makeMember("a", "A", {"Skill"});
        b.speed = 250;  // faster: acts first, drains the shared pool
        a.speed = 200;
        b.manualStats = true;
        a.manualStats = true;
        b.currentSp = 2;
        b.maxSp = 5;
        a.currentSp = 2;
        a.maxSp = 5;

        SimulationEngine engine;
        SimulationResult r = engine.runSimulation(
            {a, b}, hsrtest::singleEncounter(fragileFoe()), 500);

        TimelineStats s = analyze(r);
        CHECK(s.skills == 2);              // exactly the pool's starting points
        CHECK(s.basics == 0);
        CHECK(s.netSp == -2);
        CHECK(r.finalStats.at("team_sp") == 0);

        // Every member reports the same shared pool value.
        int aSp = r.finalStats.at("a_sp");
        int bSp = r.finalStats.at("b_sp");
        CHECK(aSp == bSp);
        CHECK(aSp == 0);
        CHECK(r.finalStats.at("team_max_sp") == 5);
    }

    // 2. Any member's Basic refills the shared pool. A single member with
    //    rotation Skill/Skill/Skill/Basic and starting SP 5 drains to 2 then
    //    refills to 3. Bounded to exactly 4 actions (maxActions=4) so the
    //    rotation does not loop and the recorded deltas are exact.
    {
        hsr::CharacterConfig a = makeMember("a", "A", {"Skill", "Skill", "Skill", "Basic"});
        a.currentSp = 5;
        a.maxSp = 5;

        SimulationEngine engine;
        SimulationResult r = engine.runSimulation(
            {a}, hsrtest::singleEncounter(hsrtest::tankyFoe()), 800, 4);

        TimelineStats s = analyze(r);
        CHECK(s.skills == 3);
        CHECK(s.basics == 1);
        CHECK(s.netSp == -2);              // -3 from Skills + 1 from Basic
        CHECK(r.finalStats.at("team_sp") == 3);
    }

    // 3. Pool floor is 0 and costs below 0 are blocked. With starting SP 0,
    //    the very first action of combat must be Basic (nothing to spend),
    //    and the running pool never dips below zero — even though both
    //    members wanted to Skill. (Individual events may still record
    //    spChange = -1 once a Basic has refilled the pool; the invariant
    //    is on the running total, not the per-event delta.)
    {
        hsr::CharacterConfig a = makeMember("a", "A", {"Skill", "Skill"});
        hsr::CharacterConfig b = makeMember("b", "B", {"Skill", "Skill"});
        a.currentSp = 0;
        b.currentSp = 0;
        a.maxSp = 5;
        b.maxSp = 5;

        SimulationEngine engine;
        SimulationResult r = engine.runSimulation({a, b},
            hsrtest::singleEncounter(hsrtest::tankyFoe()), 800, 4);

        // The first action of combat must be a Basic (empty pool).
        CHECK(!r.timeline.empty());
        CHECK(r.timeline[0].actionType == "Basic");

        // Running pool never dips below zero. Re-derive the running minimum
        // by replaying the recorded deltas forward from the configured
        // start (0), so the invariant is on the running total rather
        // than the per-event delta (a later Basic may legitimately
        // precede a Skill that records spChange = -1).
        int minPool = 0, cur = 0;
        for (const auto& ev : r.timeline) {
            cur += ev.spChange;
            if (cur < minPool) minPool = cur;
        }
        CHECK(minPool >= 0);
        CHECK(r.finalStats.at("team_sp") >= 0);
    }

    // 4. One member's spending blocks another member. Member B is faster
    //    (250) than A (200), so B acts first and drains the pool before A's
    //    first turn. With only 1 SP in the shared pool, A's first Skill is
    //    blocked and falls back to Basic even though A's own per-character
    //    SP would have been sufficient under the old per-character model.
    //    Bounded to 2 actions so only B's Skill and A's first action are
    //    observed (no later refill lets A Skill).
    {
        hsr::CharacterConfig b = makeMember("b", "B", {"Skill"});
        hsr::CharacterConfig a = makeMember("a", "A", {"Skill"});
        b.speed = 250;
        a.speed = 200;
        b.manualStats = true;
        a.manualStats = true;
        b.currentSp = 1; // only 1 in the shared pool to start
        b.maxSp = 5;
        a.currentSp = 1;
        a.maxSp = 5;

        SimulationEngine engine;
        SimulationResult r = engine.runSimulation({a, b},
            hsrtest::singleEncounter(hsrtest::tankyFoe()), 500, 2);

        // B's Skill drains the last point; A's first action must be Basic.
        bool aFirstIsBasic = false;
        bool aFirstSeen = false;
        for (const auto& ev : r.timeline) {
            if (ev.characterId == "a") {
                if (!aFirstSeen) {
                    aFirstSeen = true;
                    aFirstIsBasic = (ev.actionType == "Basic");
                }
            }
        }
        CHECK(aFirstSeen);
        CHECK(aFirstIsBasic);
    }

    if (failures == 0)
        std::printf("test_sp: all checks passed\n");
    return failures == 0 ? 0 : 1;
}