#include "SkillDatabase.h"
#include "../third_party/json.hpp"

#include <cstdio>
#include <fstream>
#include <regex>

using json = nlohmann::json;

// Data findings (character_skills_rules.json, 537 skills):
// - Blast: "Break: 20 + 10 adjacent", "#1[i]% ... to this target" +
//   "#2[i]% ... to adjacent targets" -> adjacent = slot-neighbors (slot +- 1),
//   primary at multPrimary, neighbors at multAdjacent.
// - AoE: "#1[i]% ... to all enemies" -> every active enemy at multPrimary.
// - Bounce: "#1[i]% to one designated enemy" + "#2[i] instance(s) of DMG"
//   to random enemies -> deterministic slot-order, primary + up to N others.
// - Talent: mostly Enhance/Support (non-damaging, skipped). Damaging talents
//   (Archer/Clara-counter/Ratio/Blade FUA-style counters) contain
//   "DMG equal to" and map to the FUA key. Non-damaging talents are skipped.
// - Memosprite Skill (damaging) -> own "memosprite" action key (full-turn
//   cost, like Skill). Elation Skill (damaging) -> "fua" key (instant).
// - Technique (90 entries): pre-combat preamble, no combat model -> skipped
//   with this note. Summon/Enhance/Support/Impair/Restore -> skipped.

namespace
{

bool hasDamage(const std::string& desc)
{
    return desc.find("DMG equal to") != std::string::npos ||
           desc.find("DMG up to") != std::string::npos;
}

// "Energy gain: 6" -> 6. Missing -> 0.
double parseEnergy(const std::string& desc)
{
    static const std::regex re(R"(Energy gain:\s*([0-9]+(?:\.[0-9]+)?))");
    std::smatch m;
    if (std::regex_search(desc, m, re))
        return std::stod(m[1].str());
    return 0.0;
}

// "Break: 20 + 10 adjacent" -> {20, 10}. "Break: -" / "Break: 10" handled.
std::pair<int, int> parseBreak(const std::string& desc)
{
    static const std::regex reAdj(R"(Break:\s*([0-9]+)\s*\+\s*([0-9]+)\s*adjacent)");
    static const std::regex rePlain(R"(Break:\s*([0-9]+))");
    std::smatch m;
    if (std::regex_search(desc, m, reAdj))
        return {std::stoi(m[1].str()), std::stoi(m[2].str())};
    if (std::regex_search(desc, m, rePlain))
        return {std::stoi(m[1].str()), 0};
    return {0, 0};
}

// "#2[i] instance(s) of DMG" (Bounce) -> count. Missing -> 0.
int parseBounceHits(const std::string& desc)
{
    static const std::regex re(R"(#2\[i\]\s*instance\(s\)\s*of DMG)");
    std::smatch m;
    if (std::regex_search(desc, m, re))
    {
        // The count precedes the marker: "deals #2[i] instance(s)".
        // Re-scan for the numeric run right before it.
        std::string pre = m.prefix().str();
        static const std::regex num(R"((\d+(?:\.\d+)?)\s*$)");
        std::smatch n;
        if (std::regex_search(pre, n, num))
            return static_cast<int>(std::stod(n[1].str()));
        return 1;
    }
    return 0;
}

std::string parseScaling(const std::string& desc)
{
    if (desc.find("Max HP") != std::string::npos)
        return "hp";
    if (desc.find("of DEF") != std::string::npos || desc.find("DEF to") != std::string::npos)
        return "def";
    return "atk";
}

} // namespace

bool SkillDatabase::load(const std::string& dataDir)
{
    std::string path = dataDir + "/character_skills_rules.json";
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

    if (!root.contains("skills") || !root["skills"].is_array())
        return false;

    bySlug.clear();

    for (const auto& entry : root["skills"])
    {
        if (!entry.contains("raw") || !entry["raw"].is_object())
            continue;
        const json& raw = entry["raw"];
        std::string slug = raw.value("slug", "");
        std::string skillType = raw.value("skill_type", "");
        std::string targetType = raw.value("target_type", "");
        std::string desc = raw.value("description", "");
        if (slug.empty() || desc.empty())
            continue;

        // Map data skill_type -> engine action key.
        std::string action;
        if (skillType == "Basic ATK") action = "basic";
        else if (skillType == "Skill") action = "skill";
        else if (skillType == "Ultimate") action = "ult";
        else if (skillType == "Talent") action = "fua"; // damaging only (below)
        else if (skillType == "Memosprite Skill") action = "memosprite";
        else if (skillType == "Elation Skill") action = "fua";
        else continue; // Technique / Memosprite Talent: no combat model.

        // Only damaging skills carry combat numbers. Non-damaging talents,
        // enhances, supports, impairs, restores and summons are skipped.
        if (!hasDamage(desc))
            continue;
        if (targetType != "Single Target" && targetType != "Blast" &&
            targetType != "AoE" && targetType != "Bounce")
            continue;

        ParsedSkillData data;
        data.action = action;
        data.targetType = targetType;
        auto [brk, adj] = parseBreak(desc);
        data.toughness = brk;
        data.toughnessAdjacent = adj;
        data.energy = parseEnergy(desc);
        // Multiplier level tables (#1[i]%) are NOT in this file: level_e0 /
        // base_level are skill levels (e.g. 6.0/10.0), not DMG%. So damage
        // multipliers stay 0 here (= engine keeps its configured multipliers
        // from the loadout) and are never guessed. Toughness, energy gain,
        // target type and scaling stat are the values this file provides.
        data.bounceHits = parseBounceHits(desc);
        data.scalingStat = parseScaling(desc);

        // Adjacent multiplier: #2[i]% level table is absent from this file,
        // so it stays 0 (= engine falls back to primary multiplier for
        // adjacent/bounce hits). Never guessed.
        auto& slot = bySlug[slug];
        // First damaging entry wins per action key (Basic/Skill/Ult appear
        // once per character; multiple damaging talents collapse to one FUA).
        if (slot.find(action) == slot.end())
            slot[action] = std::move(data);
    }

    return !bySlug.empty();
}

const std::unordered_map<std::string, ParsedSkillData>& SkillDatabase::skillsFor(
    const std::string& slug) const
{
    static const std::unordered_map<std::string, ParsedSkillData> empty;
    auto it = bySlug.find(slug);
    if (it == bySlug.end())
        return empty;
    return it->second;
}
