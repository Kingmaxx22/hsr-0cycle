#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// Per-skill tuning (Sec 22 DB milestone), additive schema:
//   "skills": { "basic": {"toughness": 10, "heal": 0.0, "shield": 0.0}, ... }
// Missing actions/fields mean "use engine fallback" — zeros are never
// guessed into the data file.
struct SkillTuning
{
    int toughness = 0;
    double heal = 0.0;
    double shield = 0.0;
};

struct CharacterInfo
{
    std::string id;        // "acheron" — used for both rules lookup and artwork
    std::string name;
    int rarity = 0;
    std::string element;
    std::string path;      // HSR "Path" (Nihility, Harmony, etc.)
    std::unordered_map<std::string, double> baseStats;
    std::unordered_map<std::string, SkillTuning> skills; // keys: basic/skill/ult/fua
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