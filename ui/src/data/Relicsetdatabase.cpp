#include "RelicSetDatabase.h"
#include "../third_party/json.hpp"

#include <fstream>

using json = nlohmann::json;

bool RelicSetDatabase::load(const std::string& dataDir)
{
    std::string path = dataDir + "/relic_sets_rules.json";
    std::ifstream file(path);

    if (!file)
        return false;

    json root;
    try
    {
        file >> root;
    }
    catch (const json::parse_error&)
    {
        return false;
    }

    if (!root.contains("sets"))
        return false;

    sets.clear();

    for (const auto& rec : root["sets"])
    {
        RelicSetInfo info;
        info.id = rec.value("id", "");
        info.name = rec.value("name", info.id);
        info.category = rec.value("category", "");

        if (info.id.empty())
            continue;

        sets.push_back(std::move(info));
    }

    return !sets.empty();
}

std::vector<const RelicSetInfo*> RelicSetDatabase::byCategory(const std::string& category) const
{
    std::vector<const RelicSetInfo*> result;
    for (const auto& s : sets)
        if (s.category == category)
            result.push_back(&s);
    return result;
}
