#pragma once

#include <string>
#include <vector>

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
    std::string key;
    std::string label;
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

inline const std::vector<StatOption>& mainStatOptions(GearSlot slot)
{
    static const std::vector<StatOption> head = { {"hp", "HP"} };
    static const std::vector<StatOption> hands = { {"atk", "ATK"} };

    static const std::vector<StatOption> body = {
        {"hp_pct", "HP%"},
        {"atk_pct", "ATK%"},
        {"def_pct", "DEF%"},
        {"crit_rate_pct", "CRIT Rate%"},
        {"crit_dmg_pct", "CRIT DMG%"},
        {"outgoing_healing_pct", "Outgoing Healing%"},
        {"effect_hit_rate_pct", "Effect Hit Rate%"}
    };

    static const std::vector<StatOption> feet = {
        {"hp_pct", "HP%"},
        {"atk_pct", "ATK%"},
        {"def_pct", "DEF%"},
        {"break_effect_pct", "Break Effect%"},
        {"spd", "SPD"}
    };

    static const std::vector<StatOption> sphere = {
        {"hp_pct", "HP%"},
        {"atk_pct", "ATK%"},
        {"def_pct", "DEF%"},
        {"physical_dmg_pct", "Physical DMG%"},
        {"fire_dmg_pct", "Fire DMG%"},
        {"ice_dmg_pct", "Ice DMG%"},
        {"lightning_dmg_pct", "Lightning DMG%"},
        {"wind_dmg_pct", "Wind DMG%"},
        {"quantum_dmg_pct", "Quantum DMG%"},
        {"imaginary_dmg_pct", "Imaginary DMG%"}
    };

    static const std::vector<StatOption> rope = {
        {"hp_pct", "HP%"},
        {"atk_pct", "ATK%"},
        {"def_pct", "DEF%"},
        {"break_effect_pct", "Break Effect%"},
        {"energy_regen_pct", "Energy Regen%"}
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

inline const std::vector<StatOption>& substatPool()
{
    static const std::vector<StatOption> pool = {
        {"hp", "HP"}, {"hp_pct", "HP%"},
        {"atk", "ATK"}, {"atk_pct", "ATK%"},
        {"def", "DEF"}, {"def_pct", "DEF%"},
        {"spd", "SPD"},
        {"crit_rate_pct", "CRIT Rate%"},
        {"crit_dmg_pct", "CRIT DMG%"},
        {"effect_hit_rate_pct", "Effect Hit Rate%"},
        {"effect_res_pct", "Effect RES%"},
        {"break_effect_pct", "Break Effect%"}
    };
    return pool;
}

// 5-star relic value added by a single enhancement roll.
// These are the three possible roll tiers for the current relic model.
inline const std::vector<double>& substatRollValues5Star(const std::string& key)
{
    static const std::vector<double> hp = {33.87004, 38.103795, 42.33751};
    static const std::vector<double> atk = {16.935, 19.051877, 21.168754};
    static const std::vector<double> def = {16.935, 19.051877, 21.168754};
    static const std::vector<double> hpPct = {3.456, 3.888, 4.32};
    static const std::vector<double> atkPct = {3.456, 3.888, 4.32};
    static const std::vector<double> defPct = {4.32, 4.86, 5.4};
    static const std::vector<double> spd = {2.0, 2.3, 2.6};
    static const std::vector<double> critRate = {2.592, 2.916, 3.24};
    static const std::vector<double> critDmg = {5.184, 5.832, 6.48};
    static const std::vector<double> ehr = {3.456, 3.888, 4.32};
    static const std::vector<double> effectRes = {3.456, 3.888, 4.32};
    static const std::vector<double> breakEffect = {5.184, 5.832, 6.48};
    static const std::vector<double> empty;

    if (key == "hp") return hp;
    if (key == "atk") return atk;
    if (key == "def") return def;
    if (key == "hp_pct") return hpPct;
    if (key == "atk_pct") return atkPct;
    if (key == "def_pct") return defPct;
    if (key == "spd") return spd;
    if (key == "crit_rate_pct") return critRate;
    if (key == "crit_dmg_pct") return critDmg;
    if (key == "effect_hit_rate_pct") return ehr;
    if (key == "effect_res_pct") return effectRes;
    if (key == "break_effect_pct") return breakEffect;
    return empty;
}

// 5-star maximum main-stat values at max enhancement (game constants).
// Used ONLY as editable defaults: the player can overwrite any value in
// the relic editor (Sec 22.8 — manual configuration always wins).
inline double mainStatMaxValue(const std::string& key)
{
    if (key == "hp") return 705.0;
    if (key == "atk") return 352.0;
    if (key == "hp_pct") return 43.2;
    if (key == "atk_pct") return 43.2;
    if (key == "def_pct") return 54.0;
    if (key == "crit_rate_pct") return 32.4;
    if (key == "crit_dmg_pct") return 64.8;
    if (key == "outgoing_healing_pct") return 34.56;
    if (key == "effect_hit_rate_pct") return 43.2;
    if (key == "break_effect_pct") return 64.8;
    if (key == "spd") return 25.0;
    if (key == "physical_dmg_pct") return 38.4;
    if (key == "fire_dmg_pct") return 38.4;
    if (key == "ice_dmg_pct") return 38.4;
    if (key == "lightning_dmg_pct") return 38.4;
    if (key == "wind_dmg_pct") return 38.4;
    if (key == "quantum_dmg_pct") return 38.4;
    if (key == "imaginary_dmg_pct") return 38.4;
    if (key == "energy_regen_pct") return 19.44;
    return 0.0;
}

inline bool isPercentStatKey(const std::string& key)
{
    // Percent stats are entered/displayed as percent-numbers ("43.2" + "%")
    // and divided by 100 when resolved. Matches the relic editor renderer.
    // Flat stats are "hp", "atk", "def", "spd" (no suffix).
    return key.size() > 4 &&
           key.compare(key.size() - 4, 4, "_pct") == 0;
}

inline std::string statLabel(const std::vector<StatOption>& options, const std::string& key)
{
    for (const auto& o : options)
        if (o.key == key)
            return o.label;
    return key;
}

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
    return options[(idx + 1) % options.size()].key;
}
