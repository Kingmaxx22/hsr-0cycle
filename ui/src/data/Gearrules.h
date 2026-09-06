#pragma once

#include <string>
#include <vector>

// Static HSR gearing rules: which main stats are legal on which slot, and the
// universal substat pool. This is game knowledge, not data pulled from the
// engine — it doesn't change between patches often enough to warrant a data
// file, but if it ever does, this is the only place to edit.

enum class GearSlot
{
    Head = 0,
    Hands,
    Body,
    Feet,
    PlanarSphere,
    LinkRope,
    Count
};

struct StatOption
{
    std::string key;    // stable identifier, e.g. "crit_rate_pct"
    std::string label;  // display text, e.g. "CRIT Rate%"
};

inline const char* gearSlotLabel(GearSlot slot)
{
    switch (slot)
    {
        case GearSlot::Head:         return "Head";
        case GearSlot::Hands:        return "Hands";
        case GearSlot::Body:         return "Body";
        case GearSlot::Feet:         return "Feet";
        case GearSlot::PlanarSphere: return "Planar Sphere";
        case GearSlot::LinkRope:     return "Link Rope";
        default:                     return "";
    }
}

// Main-stat options per slot. Head/Hands are each fixed to a single stat;
// the rest offer a real choice, so the UI should only make those cyclable.
inline const std::vector<StatOption>& mainStatOptions(GearSlot slot)
{
    static const std::vector<StatOption> head = { {"hp", "HP"} };
    static const std::vector<StatOption> hands = { {"atk", "ATK"} };
    static const std::vector<StatOption> body = {
        {"hp_pct", "HP%"}, {"atk_pct", "ATK%"}, {"def_pct", "DEF%"},
        {"crit_rate_pct", "CRIT Rate%"}, {"crit_dmg_pct", "CRIT DMG%"},
        {"outgoing_healing_pct", "Outgoing Healing%"}, {"effect_hit_rate_pct", "Effect Hit Rate%"}
    };
    static const std::vector<StatOption> feet = {
        {"hp_pct", "HP%"}, {"atk_pct", "ATK%"}, {"def_pct", "DEF%"}, {"spd", "SPD"}
    };
    static const std::vector<StatOption> sphere = {
        {"hp_pct", "HP%"}, {"atk_pct", "ATK%"}, {"def_pct", "DEF%"},
        {"physical_dmg_pct", "Physical DMG%"}, {"fire_dmg_pct", "Fire DMG%"},
        {"ice_dmg_pct", "Ice DMG%"}, {"lightning_dmg_pct", "Lightning DMG%"},
        {"wind_dmg_pct", "Wind DMG%"}, {"quantum_dmg_pct", "Quantum DMG%"},
        {"imaginary_dmg_pct", "Imaginary DMG%"}
    };
    static const std::vector<StatOption> rope = {
        {"hp_pct", "HP%"}, {"atk_pct", "ATK%"}, {"def_pct", "DEF%"},
        {"break_effect_pct", "Break Effect%"}, {"energy_regen_pct", "Energy Regen%"}
    };

    switch (slot)
    {
        case GearSlot::Head:         return head;
        case GearSlot::Hands:        return hands;
        case GearSlot::Body:         return body;
        case GearSlot::Feet:         return feet;
        case GearSlot::PlanarSphere: return sphere;
        case GearSlot::LinkRope:     return rope;
        default:                     return body;
    }
}

// Universal substat pool — same twelve stats regardless of slot.
inline const std::vector<StatOption>& substatPool()
{
    static const std::vector<StatOption> pool = {
        {"hp", "HP"}, {"hp_pct", "HP%"},
        {"atk", "ATK"}, {"atk_pct", "ATK%"},
        {"def", "DEF"}, {"def_pct", "DEF%"},
        {"spd", "SPD"},
        {"crit_rate_pct", "CRIT Rate%"}, {"crit_dmg_pct", "CRIT DMG%"},
        {"effect_hit_rate_pct", "Effect Hit Rate%"}, {"effect_res_pct", "Effect RES%"},
        {"break_effect_pct", "Break Effect%"}
    };
    return pool;
}

inline std::string statLabel(const std::vector<StatOption>& options, const std::string& key)
{
    for (const auto& o : options)
        if (o.key == key)
            return o.label;
    return key;
}

// Cycles to the next option's key in a StatOption list, wrapping around.
// Used for the main-stat picker where there's no "used elsewhere" constraint.
inline std::string nextStatOption(const std::vector<StatOption>& options, const std::string& current)
{
    if (options.empty())
        return current;

    size_t idx = 0;
    for (size_t i = 0; i < options.size(); ++i)
    {
        if (options[i].key == current)
        {
            idx = i;
            break;
        }
    }
    size_t nextIdx = (idx + 1) % options.size();
    return options[nextIdx].key;
}
