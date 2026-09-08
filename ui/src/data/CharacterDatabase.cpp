#include "CharacterDatabase.h"
#include "SkillDatabase.h"
#include "../third_party/json.hpp"

#include <cstdio>
#include <fstream>
#include <unordered_map>

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

    // Phase 1: parsed per-skill combat data, loaded once and overlaid per
    // character below. Best-effort: missing file leaves engine fallbacks.
    SkillDatabase skillDb;
    const bool haveSkills = skillDb.load(dataDir);

    // Phase 2: major-trace passives, loaded once and attached by slug.
    // Best-effort: missing file leaves empty trace lists.
    std::unordered_map<std::string, std::vector<MajorTrace>> tracesBySlug;
    {
        std::ifstream traceFile(dataDir + "/character_major_traces_rules.json");
        if (traceFile)
        {
            try
            {
                json traceRoot;
                traceFile >> traceRoot;
                if (traceRoot.contains("traces") && traceRoot["traces"].is_array())
                {
                    for (const auto& entry : traceRoot["traces"])
                    {
                        if (!entry.contains("raw") || !entry["raw"].is_object())
                            continue;
                        const json& raw = entry["raw"];
                        std::string slug = raw.value("slug", "");
                        if (slug.empty())
                            continue;
                        MajorTrace trace;
                        trace.slot = raw.value("trace_type", "");
                        // Display-name heuristic (display only; mechanics
                        // unaffected): trace_name is the generic "Major
                        // trace", so the real name is the leading Title-Case
                        // phrase of the description ("Red Oni When ...").
                        // Split before the first clause keyword.
                        std::string desc = raw.value("description", "");
                        static const char* markers[] = {
                            " When ", " While ", " If ", " After ", " At ",
                            " During ", " For ", " Increases ", " Whenever ",
                            " Gains ", nullptr
                        };
                        size_t cut = std::string::npos;
                        for (int mi = 0; markers[mi] != nullptr; ++mi)
                        {
                            size_t pos = desc.find(markers[mi]);
                            if (pos != std::string::npos &&
                                (cut == std::string::npos || pos < cut))
                                cut = pos;
                        }
                        if (cut != std::string::npos && cut > 0 && cut < 48)
                            trace.name = desc.substr(0, cut);
                        else
                            trace.name = trace.slot;
                        trace.description = desc;
                        tracesBySlug[slug].push_back(std::move(trace));
                    }
                }
            }
            catch (const json::parse_error& e)
            {
                fprintf(stderr, "Failed to parse traces: %s\n", e.what());
            }
        }
    }

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

        // Phase 1 overlay: parsed per-skill combat data (toughness, energy,
        // target type, scaling) keyed by slug == character id.
        if (haveSkills)
        {
            for (const auto& kv : skillDb.skillsFor(info.id))
            {
                SkillTuning& tuning = info.skills[kv.first]; // additive
                const ParsedSkillData& ps = kv.second;
                if (ps.toughness > 0) tuning.toughness = ps.toughness;
                tuning.toughnessAdjacent = ps.toughnessAdjacent;
                if (ps.energy > 0.0) tuning.energy = ps.energy;
                tuning.multPrimary = ps.multPrimary;
                tuning.multAdjacent = ps.multAdjacent;
                tuning.bounceHits = ps.bounceHits;
                tuning.targetType = ps.targetType;
                if (!ps.scalingStat.empty()) tuning.scalingStat = ps.scalingStat;
            }
        }

        if (info.id.empty())
            continue;

        auto traceIt = tracesBySlug.find(info.id);
        if (traceIt != tracesBySlug.end())
            info.traces = traceIt->second;

        // Phase 4.4: Technique preamble (display only).
        if (const auto* tech = skillDb.techniqueFor(info.id))
        {
            info.technique.slot = "Tech";
            info.technique.name = tech->name;
            info.technique.description = tech->description;
            info.hasTechnique = true;
        }

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
