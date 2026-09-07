#pragma once

#include <string>
#include <vector>

// One parsed bonus effect from pieces."2"/"4".effects[].
// JSON value is decimal-on-0..1-scale for percent types (0.12 = 12%).
struct SetEffectCondition
{
    // "damage_type" | "stacks" | "hp_threshold" | "memosprite" | "other"
    std::string kind;
    std::string description; // verbatim display text from game data
    double threshold = -1.0; // stack count / HP% cutoff; -1 = non-numeric
    std::string target;      // "self" | "team" | "single_enemy" | "all_enemies" | "" (= null)
    // Raw condition argument (e.g. damage_type {element:"lightning"}).
    // Parsed for the auto-derivation milestone; manual opt-in ignores it.
    std::string elementArg;
};

struct SetEffect
{
    std::string stat;   // e.g. "atk", "crit_rate", "damage"
    double value = 0.0; // raw JSON value (percent-scale for percent types)
    std::string kind;   // "stat_modifier" | "damage_type" | ...
    // Unconditional stat_modifier effects auto-apply. Anything else carries
    // a condition and applies ONLY via explicit user opt-in (never inferred).
    bool hasCondition = false;
    SetEffectCondition condition;
};

struct RelicSetInfo
{
    std::string id;
    std::string name;
    std::string category; // "relic" (4-slot sets) or "planar_ornament" (2-slot sets)
    std::vector<SetEffect> twoPiece;  // pieces."2".effects[]
    std::vector<SetEffect> fourPiece; // pieces."4".effects[] (relics only)
};

class RelicSetDatabase
{
public:
    // dataDir = path to engine/hsr_engine/data
    bool load(const std::string& dataDir);

    std::vector<const RelicSetInfo*> byCategory(const std::string& category) const;
    const std::vector<RelicSetInfo>& all() const { return sets; }

    const RelicSetInfo* getById(const std::string& id) const;
    const RelicSetInfo* getByName(const std::string& name) const;
    const RelicSetInfo* get(const std::string& idOrName) const;

private:
    std::vector<RelicSetInfo> sets;
};
