#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct CharacterInfo
{
    std::string id;        // "acheron" — used for both rules lookup and artwork
    std::string name;
    int rarity = 0;
    std::string element;
    std::string path;      // HSR "Path" (Nihility, Harmony, etc.)
    std::unordered_map<std::string, double> baseStats;
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