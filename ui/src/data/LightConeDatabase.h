#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct LightConeInfo
{
    std::string id;               // e.g. "along-the-passing-shore"
    std::string assetId;          // e.g. "along_the_passing_shore" (for AssetManager)
    std::string name;             // e.g. "Along the Passing Shore"
    int rarity = 5;               // 3, 4, 5
    std::string path;             // "Destruction", "Hunt", "Erudition", "Harmony", "Nihility", "Preservation", "Abundance", "Remembrance", "Elation"
    int hp = 0;                   // Base HP at Lv 80
    int atk = 0;                  // Base ATK at Lv 80
    int def = 0;                  // Base DEF at Lv 80
    std::string effectDescription;// Passive ability text
};

class LightConeDatabase
{
public:
    // dataDir = path to engine/hsr_engine/data
    bool load(const std::string& dataDir);

    const LightConeInfo* get(const std::string& idOrName) const;
    const LightConeInfo* getById(const std::string& id) const;
    const LightConeInfo* getByName(const std::string& name) const;
    std::vector<const LightConeInfo*> byPath(const std::string& path) const;
    const std::vector<LightConeInfo>& all() const { return lightCones; }

private:
    std::vector<LightConeInfo> lightCones;
    std::unordered_map<std::string, size_t> idIndex;
    std::unordered_map<std::string, size_t> nameIndex;
};
