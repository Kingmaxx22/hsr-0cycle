#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// Per-skill tuning (Sec 22 DB milestone + Phase 1 skill data), additive:
//   "skills": { "basic": {"toughness": 10, "heal": 0.0, "shield": 0.0}, ... }
// Missing actions/fields mean "use engine fallback" - zeros are never
// guessed into the data file. targetType is one of "", "Single Target",
// "Blast", "AoE", "Bounce" ("" = single-target fallback).
struct SkillTuning
{
    int toughness = 0;
    int toughnessAdjacent = 0;
    double heal = 0.0;
    double shield = 0.0;
    double energy = 0.0;
    double multPrimary = 0.0;
    double multAdjacent = 0.0;
    int bounceHits = 0;
    std::string targetType;
    std::string scalingStat;
};

struct MajorTrace
{
    std::string slot;       // "A2", "A4", "A6"
    std::string name;
    std::string description;
};

// Eidolon level (Phase 3): parsed from character_eidolons_rules.json
// (generated from character_eidolons.csv). skillLevels lists the abilities
// this Eidolon raises ("Skill Lv. +2" style) — used data-driven to select
// boosted manual scaling values. All other mechanics travel as
// informational notes only (§29, no auto-resolution).
struct EidolonLevel
{
    int eidolon = 0;        // 1..6
    std::string title;
    std::string description;
    std::vector<std::string> skillLevels; // e.g. {"Ultimate", "Basic ATK"}
};

struct CharacterInfo
{
    std::string id;        // "acheron" - used for both rules lookup and artwork
    std::string name;
    int rarity = 0;
    std::string element;
    std::string path;      // HSR "Path" (Nihility, Harmony, etc.)
    std::unordered_map<std::string, double> baseStats;
    std::unordered_map<std::string, SkillTuning> skills; // keys: basic/skill/ult/fua/memosprite
    std::vector<MajorTrace> traces; // A2/A4/A6, file order (Phase 2)
    // Phase 4.4: Technique preamble description (display only; exactly one
    // per character when the skill file carries it).
    MajorTrace technique;
    bool hasTechnique = false;
    // Phase 3: Eidolon levels, ascending (empty when the rules file
    // lacks this slug — engine fallbacks apply).
    std::vector<EidolonLevel> eidolons;
    // DoT base chance default from dot_base_chance.csv (0 = no kit DoT).
    // Feeds CharacterConfig.breakDotChance; the DoT type/turns/scale stay
    // user-configured (tryApplyBreakDot needs all of them).
    double dotBaseChance = 0.0;
};

class CharacterDatabase
{
public:
    // dataDir = path to engine/hsr_engine/data
    bool load(const std::string& dataDir);

    const CharacterInfo* get(const std::string& id) const;
    const std::vector<CharacterInfo>& all() const { return characters; }

private:
    std::vector<CharacterInfo> characters;
    std::unordered_map<std::string, size_t> idIndex;
};