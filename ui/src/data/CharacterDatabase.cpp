#include "CharacterDatabase.h"
#include "../third_party/json.hpp"

#include <cstdio>
#include <fstream>

using json = nlohmann::json;

// Prydwen character page titles look like "Acheron Best Build Guide | Honkai: Star Rail".
// We only want "Acheron" for display; the "id" field is already the clean slug.
static std::string cleanDisplayName(const std::string& rawName)
{
    auto pos = rawName.find(" Best Build Guide");
    if (pos != std::string::npos)
        return rawName.substr(0, pos);
    return rawName;
}

bool CharacterDatabase::load(const std::string& dataDir)
{
    std::string path = dataDir + "/characters_rules.json";
    std::ifstream file(path);

    if (!file)
        return false;

    json root;
    try
    {
        file >> root;
    }
    catch (const json::parse_error& e)
    {
        fprintf(stderr, "Failed to parse %s: %s\n", path.c_str(), e.what());
        return false;
    }

    if (!root.contains("characters"))
        return false;

    characters.clear();
    idIndex.clear();

    for (const auto& rec : root["characters"])
    {
        CharacterInfo info;
        info.id = rec.value("id", "");
        info.name = cleanDisplayName(rec.value("name", info.id));
        info.rarity = rec.value("rarity", 0);
        info.element = rec.value("element", "");
        info.path = rec.value("path", "");

        if (rec.contains("base_stats"))
        {
            for (auto& [key, val] : rec["base_stats"].items())
            {
                if (val.is_number())
                    info.baseStats[key] = val.get<double>();
            }
        }

        // Optional per-skill tuning (additive; absent = engine fallbacks).
        if (rec.contains("skills") && rec["skills"].is_object())
        {
            for (auto& [action, data] : rec["skills"].items())
            {
                if (!data.is_object())
                    continue;
                SkillTuning tuning;
                tuning.toughness = data.value("toughness", 0);
                tuning.heal = data.value("heal", 0.0);
                tuning.shield = data.value("shield", 0.0);
                info.skills[action] = tuning;
            }
        }

        if (info.id.empty())
            continue;

        idIndex[info.id] = characters.size();
        characters.push_back(std::move(info));
    }

    return !characters.empty();
}

const CharacterInfo* CharacterDatabase::get(const std::string& id) const
{
    auto it = idIndex.find(id);
    if (it == idIndex.end())
        return nullptr;
    return &characters[it->second];
}
