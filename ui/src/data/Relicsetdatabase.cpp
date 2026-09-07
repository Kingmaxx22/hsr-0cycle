#include "Relicsetdatabase.h"
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

    auto parseEffects = [](const json& piece) {
        std::vector<SetEffect> out;
        std::string rawText = piece.value("raw_text", "");
        if (!piece.contains("effects") || !piece["effects"].is_array())
            return out;
        for (const auto& fx : piece["effects"])
        {
            SetEffect effect;
            effect.stat = fx.value("stat", "");
            effect.value = fx.value("value", 0.0);
            effect.kind = fx.value("kind", "");
            if (effect.stat.empty())
                continue;
            // A nested "condition" object (e.g. {"kind":"damage_type",
            // "args":{"element":"lightning"}} on elemental 2pc sets) gates
            // the effect behind manual opt-in. So does any unknown kind.
            // Never infer activation from encounter state this phase.
            // NOTE: condition.args (e.g. the element) is preserved in the
            // raw data for the deferred auto-derivation milestone; only
            // the schema fields below are parsed now.
            bool hasCondObject = fx.contains("condition") && fx["condition"].is_object();
            if (hasCondObject || effect.kind != "stat_modifier")
            {
                effect.hasCondition = true;
                effect.condition.description = rawText;
                std::string condKind = hasCondObject ? fx["condition"].value("kind", "") : "";
                if (condKind == "damage_type" || condKind == "stacks" ||
                    condKind == "hp_threshold" || condKind == "memosprite")
                    effect.condition.kind = condKind;
                else if (effect.kind == "damage_type")
                    effect.condition.kind = "damage_type";
                else
                    effect.condition.kind = "other";
                std::string target = fx.value("target", "");
                if (target == "self" || target == "team" ||
                    target == "single_enemy" || target == "all_enemies")
                    effect.condition.target = target;
                if (hasCondObject && fx["condition"].contains("args") &&
                    fx["condition"]["args"].is_object())
                {
                    effect.condition.elementArg =
                        fx["condition"]["args"].value("element", "");
                }
            }
            out.push_back(std::move(effect));
        }
        return out;
    };

    for (const auto& rec : root["sets"])
    {
        RelicSetInfo info;
        info.id = rec.value("id", "");
        info.name = rec.value("name", info.id);
        info.category = rec.value("category", "");

        if (info.id.empty())
            continue;

        if (rec.contains("pieces") && rec["pieces"].is_object())
        {
            const json& pieces = rec["pieces"];
            if (pieces.contains("2") && pieces["2"].is_object())
                info.twoPiece = parseEffects(pieces["2"]);
            if (pieces.contains("4") && pieces["4"].is_object())
                info.fourPiece = parseEffects(pieces["4"]);
        }

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

const RelicSetInfo* RelicSetDatabase::getById(const std::string& id) const
{
    for (const auto& s : sets)
        if (s.id == id)
            return &s;
    return nullptr;
}

const RelicSetInfo* RelicSetDatabase::getByName(const std::string& name) const
{
    for (const auto& s : sets)
        if (s.name == name)
            return &s;
    return nullptr;
}

const RelicSetInfo* RelicSetDatabase::get(const std::string& idOrName) const
{
    if (idOrName.empty())
        return nullptr;
    const auto* byId = getById(idOrName);
    if (byId)
        return byId;
    return getByName(idOrName);
}
