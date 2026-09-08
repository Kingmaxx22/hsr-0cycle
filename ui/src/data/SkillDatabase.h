#pragma once

#include <string>
#include <unordered_map>

// Parsed per-skill combat data from character_skills_rules.json (Phase 1).
// Additive schema: absent entries / zero values mean "use engine fallback".
// Nothing is guessed: if a value cannot be parsed it stays zero.
struct ParsedSkillData
{
    // Engine action key this skill maps to: basic/skill/ult/fua/memosprite.
    std::string action;
    // Targeting: "Single Target", "Blast", "AoE", "Bounce" (others skipped).
    std::string targetType;
    int toughness = 0;          // Primary-target toughness damage (Break:)
    int toughnessAdjacent = 0;  // Adjacent-target toughness ("+ N adjacent")
    double energy = 0.0;        // Energy gain:
    double multPrimary = 0.0;   // #1[i]% damage multiplier (as decimal)
    double multAdjacent = 0.0;  // #2[i]% adjacent/bounce multiplier (as decimal)
    int bounceHits = 0;         // Bounce instance count (#2[i] instance(s))
    std::string scalingStat = "atk";
};

class SkillDatabase
{
public:
    // dataDir = path to engine/hsr_engine/data. Best-effort: returns false
    // when the file is missing/unparseable; callers keep engine fallbacks.
    // Merges parsed skills into the per-character skill map keyed by slug.
    bool load(const std::string& dataDir);

    const std::unordered_map<std::string, ParsedSkillData>& skillsFor(
        const std::string& slug) const;

    // Phase 4.4: Technique description per slug (at most one each).
    // Pre-combat preamble with free-form mechanics: parsed and stored for
    // display only, never auto-resolved into combat effects.
    struct TechniqueInfo
    {
        std::string name;
        std::string description;
    };
    const TechniqueInfo* techniqueFor(const std::string& slug) const;

    // DoT base chances (dot_base_chance.csv, hsr-optimizer extraction):
    // slug -> base chance (0..1). Only literal rows are stored;
    // computed rows (Black Swan, Guinaifen `dotChance` variables) are
    // skipped with a note — never guessed. Missing slug = no kit DoT.
    double dotChanceFor(const std::string& slug) const;

private:
    // slug -> (action key -> parsed data)
    std::unordered_map<std::string,
        std::unordered_map<std::string, ParsedSkillData>> bySlug;
    std::unordered_map<std::string, TechniqueInfo> techniques;
    std::unordered_map<std::string, double> dotChances;
};
