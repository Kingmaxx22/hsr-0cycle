// Phase 3 verification: Eidolon rules, E-gated scaling selection,
// and DoT base-chance defaults. Plain asserts, zero deps.
#include "data/CharacterDatabase.h"
#include "data/Characterloadout.h"
#include "data/LightConeDatabase.h"
#include "data/LoadoutResolver.h"
#include "data/RelicSetDatabase.h"
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
    CharacterDatabase characters;
    CHECK(characters.load("engine/hsr_engine/data"));

    // Eidolon rules: Acheron has 6 levels with titles + skill raises.
    const CharacterInfo* acheron = characters.get("acheron");
    CHECK(acheron != nullptr);
    if (acheron != nullptr)
    {
        CHECK(acheron->eidolons.size() == 6);
        if (acheron->eidolons.size() == 6)
        {
            CHECK(acheron->eidolons[0].eidolon == 1);
            CHECK(acheron->eidolons[0].title == "Silenced Sky Spake Sooth");
            CHECK(acheron->eidolons[0].skillLevels.empty());
            CHECK(acheron->eidolons[2].eidolon == 3);
            CHECK(acheron->eidolons[2].skillLevels.size() == 2);
            CHECK(acheron->eidolons[4].eidolon == 5);
            bool hasSkill = false, hasTalent = false;
            for (const auto& s : acheron->eidolons[4].skillLevels)
            {
                if (s == "Skill") hasSkill = true;
                if (s == "Talent") hasTalent = true;
            }
            CHECK(hasSkill);
            CHECK(hasTalent);
        }
    }

    LightConeDatabase lightCones;
    lightCones.load("engine/hsr_engine/data");
    RelicSetDatabase relicSets;
    relicSets.load("engine/hsr_engine/data");

    if (acheron != nullptr)
    {
        // E-gated scaling: Skill boosts at E5. E4 -> base, E5 -> boosted.
        CharacterLoadout lo;
        lo.scalingTables["skill"] = {2.0, 2.3};
        lo.scalingTables["ult"] = {3.0, 3.4}; // Ult boosts at E3
        lo.eidolonLevel = 4;
        hsr::CharacterConfig e4;
        loadout::applyToCharacterConfig(e4, *acheron, lo, lightCones,
                                        relicSets);
        CHECK(e4.skillActions["Skill"].damageMultiplier == 2.0);
        CHECK(e4.skillActions["Ult"].damageMultiplier == 3.4);
        // Non-skill E4 annotated; E6 (locked) absent.
        bool hasE4 = false, hasE6 = false;
        for (const auto& note : e4.passiveNotes)
        {
            if (note.find("Shrined Fire") != std::string::npos) hasE4 = true;
            if (note.rfind("E6 ", 0) == 0) hasE6 = true;
        }
        CHECK(hasE4);
        CHECK(!hasE6);

        lo.eidolonLevel = 5;
        hsr::CharacterConfig e5;
        loadout::applyToCharacterConfig(e5, *acheron, lo, lightCones,
                                        relicSets);
        CHECK(e5.skillActions["Skill"].damageMultiplier == 2.3);

        // E0: base values, no Eidolon notes.
        lo.eidolonLevel = 0;
        hsr::CharacterConfig e0;
        loadout::applyToCharacterConfig(e0, *acheron, lo, lightCones,
                                        relicSets);
        CHECK(e0.skillActions["Skill"].damageMultiplier == 2.0);
        CHECK(e0.passiveNotes.empty());
    }

    // DoT base chances: literals resolve, computed rows are skipped.
    SkillDatabase skillDb;
    CHECK(skillDb.load("engine/hsr_engine/data"));
    CHECK(skillDb.dotChanceFor("kafka") == 1.0);
    CHECK(skillDb.dotChanceFor("sampo") == 0.65);
    CHECK(skillDb.dotChanceFor("asta") == 0.80);
    CHECK(skillDb.dotChanceFor("black-swan") == 0.0); // computed: skipped
    CHECK(skillDb.dotChanceFor("guinaifen") == 0.0);  // computed: skipped
    CHECK(skillDb.dotChanceFor("no-such-character") == 0.0);

    // Resolver carries the DB chance as the break-DoT default.
    const CharacterInfo* kafka = characters.get("kafka");
    CHECK(kafka != nullptr);
    if (kafka != nullptr)
    {
        CHECK(kafka->dotBaseChance == 1.0);
        CharacterLoadout lo;
        hsr::CharacterConfig config;
        loadout::applyToCharacterConfig(config, *kafka, lo, lightCones,
                                        relicSets);
        CHECK(config.breakDotChance == 1.0);
    }

    if (failures == 0)
        std::printf("test_eidolons: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
