// Phase 1 verification: SkillDatabase parsing of character_skills_rules.json.
// Plain asserts, zero deps. Run from the repo root (CTest sets it).
#include "data/SkillDatabase.h"

#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { \
    if (!(cond)) { \
        ++failures; \
        std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

int main()
{
    SkillDatabase db;
    CHECK(db.load("engine/hsr_engine/data"));

    // Acheron Skill is Blast with "Break: 20 + 10 adjacent", energy 0.
    const auto& acheron = db.skillsFor("acheron");
    CHECK(!acheron.empty());
    auto it = acheron.find("skill");
    CHECK(it != acheron.end());
    if (it != acheron.end())
    {
        CHECK(it->second.targetType == "Blast");
        CHECK(it->second.toughness == 20);
        CHECK(it->second.toughnessAdjacent == 10);
        CHECK(it->second.energy == 0.0);
    }

    // Acheron Basic is Single Target, Break 10.
    auto basic = acheron.find("basic");
    CHECK(basic != acheron.end());
    if (basic != acheron.end())
    {
        CHECK(basic->second.targetType == "Single Target");
        CHECK(basic->second.toughness == 10);
    }

    // Damaging talents map to FUA (e.g. ratio has a damaging Talent).
    const auto& ratio = db.skillsFor("dr-ratio");
    auto fua = ratio.find("fua");
    CHECK(fua != ratio.end());
    if (fua != ratio.end())
        CHECK(fua->second.toughness > 0);

    // Technique entries never surface as actions.
    for (const auto& kv : acheron)
        CHECK(kv.first != "technique");

    // Unknown slug: empty, no crash.
    CHECK(db.skillsFor("no-such-character").empty());
    CHECK(db.techniqueFor("no-such-character") == nullptr);

    // Phase 4.4: Techniques stored per slug (display only).
    const auto* tech = db.techniqueFor("acheron");
    CHECK(tech != nullptr);
    if (tech != nullptr)
    {
        CHECK(tech->name == "Quadrivalent Ascendance");
        CHECK(!tech->description.empty());
    }

    // At least one AoE and one Bounce skill parsed across the file.
    // (Spot-check via known AoE: castorice memosprite AoE.)
    const auto& castorice = db.skillsFor("castorice");
    bool hasAoe = false;
    for (const auto& kv : castorice)
        if (kv.second.targetType == "AoE")
            hasAoe = true;
    CHECK(hasAoe);

    if (failures == 0)
        std::printf("test_skilldb: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
