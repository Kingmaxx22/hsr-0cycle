// Phase 2 verification: major-trace parsing, LC passive opt-ins, and
// resolver informational notes. Plain asserts, zero deps.
#include "data/CharacterDatabase.h"
#include "data/Characterloadout.h"
#include "data/LightConeDatabase.h"
#include "data/LoadoutResolver.h"
#include "data/RelicSetDatabase.h"

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

    // Acheron has exactly A2/A4/A6 with display names parsed from text.
    const CharacterInfo* acheron = characters.get("acheron");
    CHECK(acheron != nullptr);
    if (acheron != nullptr)
    {
        CHECK(acheron->traces.size() == 3);
        if (acheron->traces.size() == 3)
        {
            CHECK(acheron->traces[0].slot == "A2");
            CHECK(acheron->traces[1].slot == "A4");
            CHECK(acheron->traces[2].slot == "A6");
            CHECK(acheron->traces[0].name == "Red Oni");
            CHECK(acheron->traces[1].name == "The Abyss");
            CHECK(acheron->traces[2].name == "Thunder Core");
            for (const auto& trace : acheron->traces)
                CHECK(!trace.description.empty());
        }
    }

    // Loadout opt-ins default OFF.
    CharacterLoadout loadout;
    CHECK(loadout.traceActive.empty());
    CHECK(loadout.lcPassiveActive == false);

    // Resolver: nothing enabled -> no notes (mechanics untouched).
    LightConeDatabase lightCones;
    CHECK(lightCones.load("engine/hsr_engine/data"));
    RelicSetDatabase relicSets;
    relicSets.load("engine/hsr_engine/data");
    if (acheron != nullptr)
    {
        hsr::CharacterConfig plain;
        loadout::applyToCharacterConfig(plain, *acheron, loadout,
                                        lightCones, relicSets);
        CHECK(plain.passiveNotes.empty());
        CHECK(plain.critRate == 0.05); // base game constant, unaffected

        // Resolver: enabled trace + LC passive -> informational notes only.
        loadout.traceActive["A2"] = true;
        loadout.lightConeId = "a-dream-scented-in-wheat";
        loadout.lcPassiveActive = true;
        hsr::CharacterConfig noted;
        loadout::applyToCharacterConfig(noted, *acheron, loadout,
                                        lightCones, relicSets);
        CHECK(noted.passiveNotes.size() == 2);
        CHECK(noted.critRate == 0.05); // still no numeric effect inferred
        bool hasTrace = false, hasLc = false;
        for (const auto& note : noted.passiveNotes)
        {
            if (note.find("Red Oni") != std::string::npos)
                hasTrace = true;
            if (note.find("LC passive") != std::string::npos)
                hasLc = true;
        }
        CHECK(hasTrace);
        CHECK(hasLc);
    }

    if (failures == 0)
        std::printf("test_passives: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
