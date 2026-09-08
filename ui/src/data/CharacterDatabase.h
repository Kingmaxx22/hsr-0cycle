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