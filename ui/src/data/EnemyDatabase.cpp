#include "EnemyDatabase.h"
#include "../third_party/json.hpp"
#include "raylib.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>

using json = nlohmann::json;

namespace
{
    std::string trim(std::string value)
    {
        const auto notSpace = [](unsigned char c)
        {
            return !std::isspace(c);
        };

        value.erase(value.begin(),
                    std::find_if(value.begin(), value.end(), notSpace));
        value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(),
                    value.end());
        return value;
    }

    std::string cleanName(std::string value)
    {
        value = trim(std::move(value));
        while (value.size() >= 2 &&
               ((value.front() == '"' && value.back() == '"') ||
                (value.front() == '\'' && value.back() == '\'')))
        {
            value = trim(value.substr(1, value.size() - 2));
        }
        return value;
    }

    bool isNumericName(const std::string& value)
    {
        if (value.empty())
            return true;

        return std::all_of(value.begin(), value.end(), [](unsigned char c)
        {
            return std::isdigit(c);
        });
    }

    std::string stringOr(const json& object, const char* key)
    {
        if (!object.contains(key) || object[key].is_null())
            return {};

        if (object[key].is_string())
            return object[key].get<std::string>();

        if (object[key].is_number())
            return object[key].dump();

        return {};
    }

    double parseNumber(std::string value, double fallback = 0.0)
    {
        value = trim(std::move(value));
        if (value.empty())
            return fallback;

        value.erase(std::remove(value.begin(), value.end(), ','), value.end());
        if (!value.empty() && value.back() == '%')
            value.pop_back();

        try
        {
            size_t parsed = 0;
            const double result = std::stod(value, &parsed);
            if (parsed == value.size())
                return result;
        }
        catch (...)
        {
        }

        return fallback;
    }

    double numberOr(const json& object, const char* key, double fallback = 0.0)
    {
        if (!object.contains(key) || object[key].is_null())
            return fallback;

        if (object[key].is_number())
            return object[key].get<double>();

        if (object[key].is_string())
            return parseNumber(object[key].get<std::string>(), fallback);

        return fallback;
    }

    bool boolOr(const json& object, const char* key, bool fallback = false)
    {
        if (!object.contains(key) || object[key].is_null())
            return fallback;

        if (object[key].is_boolean())
            return object[key].get<bool>();

        if (object[key].is_string())
        {
            std::string value = object[key].get<std::string>();
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char c)
                           {
                               return static_cast<char>(std::tolower(c));
                           });
            if (value == "true" || value == "1")
                return true;
            if (value == "false" || value == "0")
                return false;
        }

        return fallback;
    }

    std::string variantsText(const json& rec)
    {
        return stringOr(rec, "variants");
    }

    // The nanoka.cc scrape stores the real monster name in the first
    // `ID <monster id> ... Faction:` block. The JSON `name` field may instead
    // be "Version Diff" or even the numeric monster ID.
    std::string extractSourceSegment(const std::string& variants,
                                     const std::string& id)
    {
        const std::string marker = "ID " + id + " ";
        const size_t start = variants.find(marker);
        if (start == std::string::npos)
            return {};

        const size_t begin = start + marker.size();
        const size_t end = variants.find(" || ID ", begin);
        if (end == std::string::npos)
            return variants.substr(begin);

        return variants.substr(begin, end - begin);
    }

    std::string extractSourceName(const std::string& variants,
                                  const std::string& id)
    {
        const std::string segment = extractSourceSegment(variants, id);
        if (segment.empty())
            return {};

        const size_t faction = segment.find(" Faction:");
        std::string name = faction == std::string::npos
            ? segment
            : segment.substr(0, faction);

        return cleanName(name);
    }

    std::string extractSourceRating(const std::string& variants,
                                    const std::string& id)
    {
        const std::string segment = extractSourceSegment(variants, id);
        const size_t ratingPos = segment.find("Rating:");
        if (ratingPos == std::string::npos)
            return {};

        const size_t valueBegin = ratingPos + 7;
        const size_t weaknessPos = segment.find(" Weakness", valueBegin);
        const size_t end = weaknessPos == std::string::npos
            ? segment.size()
            : weaknessPos;

        return trim(segment.substr(valueBegin, end - valueBegin));
    }

    bool isWeaknessToken(const std::string& token)
    {
        return token == "Physical" ||
               token == "Fire" ||
               token == "Ice" ||
               token == "Thunder" ||
               token == "Lightning" ||
               token == "Wind" ||
               token == "Quantum" ||
               token == "Imaginary";
    }

    std::string canonicalElement(std::string value)
    {
        if (value == "Thunder")
            return "Lightning";
        return value;
    }

    std::vector<std::string> extractSourceWeaknesses(const std::string& variants,
                                                      const std::string& id)
    {
        const std::string segment = extractSourceSegment(variants, id);
        const size_t weaknessPos = segment.find("Weakness ");
        if (weaknessPos == std::string::npos)
            return {};

        const size_t begin = weaknessPos + 9;
        const size_t end = segment.find(" ||", begin);
        const std::string value = end == std::string::npos
            ? segment.substr(begin)
            : segment.substr(begin, end - begin);

        std::vector<std::string> result;
        std::istringstream stream(value);
        std::string token;

        while (stream >> token)
        {
            token = canonicalElement(trim(token));
            if (isWeaknessToken(token) &&
                std::find(result.begin(), result.end(), token) == result.end())
            {
                result.push_back(token);
            }
        }

        return result;
    }

    bool ratingSaysBoss(const std::string& rating)
    {
        return rating.find("Boss") != std::string::npos ||
               rating.find("boss") != std::string::npos;
    }

    bool ratingSaysElite(const std::string& rating)
    {
        return rating.find("Elite") != std::string::npos ||
               rating.find("elite") != std::string::npos;
    }
}

bool EnemyDatabase::load(const std::string& dataDir)
{
    const std::string path = dataDir + "/monsters_rules.json";
    std::ifstream file(path);

    if (!file)
    {
        TraceLog(LOG_ERROR, "Failed to open monster rules: %s", path.c_str());
        return false;
    }

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

    const json* records = nullptr;
    if (root.is_array())
        records = &root;
    else if (root.contains("monsters") && root["monsters"].is_array())
        records = &root["monsters"];
    else
    {
        TraceLog(LOG_ERROR, "Monster rules have an unsupported JSON shape: %s", path.c_str());
        return false;
    }

    enemies.clear();
    idIndex.clear();

    for (const auto& rec : *records)
    {
        if (!rec.is_object())
            continue;

        EnemyInfo enemy;
        enemy.id = stringOr(rec, "id");
        enemy.name = cleanName(stringOr(rec, "name"));
        enemy.rating = trim(stringOr(rec, "rating"));
        enemy.assetId = stringOr(rec, "asset_id");
        enemy.assetPath = stringOr(rec, "asset_path");

        if (enemy.id.empty())
            continue;

        const std::string variants = variantsText(rec);

        const std::string sourceName = extractSourceName(variants, enemy.id);
        if (!sourceName.empty() &&
            (enemy.name.empty() ||
             enemy.name == "Version Diff" ||
             isNumericName(enemy.name)))
        {
            enemy.name = sourceName;
        }

        if (enemy.name.empty())
            enemy.name = enemy.id;

        const std::string sourceRating = extractSourceRating(variants, enemy.id);
        if (!sourceRating.empty() &&
            (enemy.rating.empty() || enemy.rating == "Version Diff"))
        {
            enemy.rating = sourceRating;
        }

        if (enemy.assetId.empty())
            enemy.assetId = "Monster_" + enemy.id;

        if (enemy.assetPath.empty())
            enemy.assetPath = "prydwen_assets/enemies/Monster_" + enemy.id + ".png";

        // Support both the normalized/base_stats schema and the raw scraped
        // top-level stat fields.
        const json* stats = nullptr;
        if (rec.contains("base_stats") && rec["base_stats"].is_object())
            stats = &rec["base_stats"];

        const auto readStat = [&](const char* key) -> double
        {
            const double top = numberOr(rec, key, 0.0);
            if (top != 0.0)
                return top;
            return stats ? numberOr(*stats, key, 0.0) : 0.0;
        };

        enemy.level = static_cast<int>(numberOr(rec, "level", 0.0));
        if (enemy.level == 0 && rec.contains("base_stats") && rec["base_stats"].is_object())
            enemy.level = static_cast<int>(numberOr(rec["base_stats"], "level", 0.0));

        enemy.hp = readStat("hp");
        enemy.atk = readStat("atk");
        enemy.def = readStat("def");
        enemy.spd = readStat("spd");
        enemy.toughness = numberOr(rec, "toughness");
        enemy.effectHitRate = numberOr(rec, "effect_hit_rate");
        enemy.effectRes = numberOr(rec, "effect_res");
        enemy.minimumRes = numberOr(rec, "minimum_res");
        enemy.critDmg = numberOr(rec, "crit_dmg");
        enemy.firstTurnDelay = numberOr(rec, "first_turn_delay");

        enemy.isBoss = boolOr(rec, "is_boss");
        enemy.isElite = boolOr(rec, "is_elite");

        if (rec.contains("is_boss_or_elite") && rec["is_boss_or_elite"].is_boolean())
        {
            const bool value = rec["is_boss_or_elite"].get<bool>();
            enemy.isBoss = enemy.isBoss || value;
            enemy.isElite = enemy.isElite || value;
        }

        if (!sourceRating.empty())
            enemy.rating = sourceRating;

        if (!enemy.rating.empty())
        {
            enemy.isBoss = enemy.isBoss || ratingSaysBoss(enemy.rating);
            enemy.isElite = enemy.isElite || ratingSaysElite(enemy.rating);
        }

        // Prefer the clean weakness list extracted from the source segment.
        // The raw `weakness` field often contains the whole scraped page and is
        // therefore unsafe to display directly.
        enemy.weaknesses = extractSourceWeaknesses(variants, enemy.id);

        if (enemy.weaknesses.empty() &&
            rec.contains("weaknesses") && rec["weaknesses"].is_array())
        {
            for (const auto& weak : rec["weaknesses"])
            {
                if (weak.is_string())
                    enemy.weaknesses.push_back(canonicalElement(weak.get<std::string>()));
            }
        }

        if (enemy.weaknesses.empty() && rec.contains("weakness") && rec["weakness"].is_string())
        {
            std::istringstream stream(rec["weakness"].get<std::string>());
            std::string token;
            while (stream >> token)
            {
                token = canonicalElement(token);
                if (isWeaknessToken(token) &&
                    std::find(enemy.weaknesses.begin(), enemy.weaknesses.end(), token) == enemy.weaknesses.end())
                {
                    enemy.weaknesses.push_back(token);
                }
            }
        }

        if (rec.contains("resistances") && rec["resistances"].is_object())
        {
            for (auto& [element, value] : rec["resistances"].items())
            {
                if (value.is_number())
                    enemy.resistances[element] = value.get<double>();
            }
        }

        idIndex[enemy.id] = enemies.size();
        enemies.push_back(std::move(enemy));
    }

    TraceLog(LOG_INFO, "Loaded %zu enemies", enemies.size());
    return !enemies.empty();
}

const EnemyInfo* EnemyDatabase::get(const std::string& id) const
{
    const auto it = idIndex.find(id);
    return it == idIndex.end() ? nullptr : &enemies[it->second];
}
