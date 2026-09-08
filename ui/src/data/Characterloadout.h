#pragma once

#include "Gearrules.h"

#include <array>
#include <cmath>
#include <string>
#include <unordered_map>

// A single substat roll on a gear piece. statKey is empty when the row
// hasn't been assigned a stat yet; valueText is a raw, user-typed number
// buffer (not parsed/validated here — the rules engine does that later).
struct SubstatRoll
{
    std::string statKey;
    std::string valueText;
};

struct GearPiece
{
    std::string mainStat;
    // Editable main-stat value (same entry convention as substat valueText:
    // percent-number for % stats, raw number for flat stats). Defaults to
    // the 5-star max; the player can overwrite it (Sec 22.8).
    std::string mainStatValueText;
    std::array<SubstatRoll, 4> substats;
};

// "Other stat bonuses" (Sec 22.2): anything not covered by base stats, Light
// Cone, or gear — e.g. team buffs, technique bonuses, manual corrections.
// Percent fields are stored as decimals (0.15 = 15%); flatSpd is raw Speed.
struct OtherStatBonuses
{
    double hpPct = 0.0;
    double atkPct = 0.0;
    double defPct = 0.0;
    double flatSpd = 0.0;
    double critRate = 0.0;
    double critDmg = 0.0;
    double elemDmgPct = 0.0;
    double resPen = 0.0;
    double ehr = 0.0;
    // Break-DMG-Increase (Eidolon-gated, e.g. Fugue E4): user-asserted,
    // no gear/DB source. Decimals.
    double breakDmgIncrease = 0.0;
};

// Manual base-stat override (Sec 22.1/22.8): lets a player without complete
// character data type base values directly. Ignored unless useOverride.
struct ManualBaseStats
{
    bool useOverride = false;
    double hp = 0.0;
    double atk = 0.0;
    double def = 0.0;
    double spd = 0.0;
};

struct CharacterLoadout
{
    // Light Cone equipped on this character
    std::string lightConeId;
    int lightConeSuperimposition = 1; // 1 to 5 (S1..S5)

    // Gear
    std::array<GearPiece, static_cast<size_t>(GearSlot::Count)> gear;

    // true  = a single 4-piece relic set (Head/Hands/Body/Feet all match)
    // false = two different 2-piece relic sets
    bool relicFourPiece = true;
    std::string relicSetA;   // the 4pc set, or the first 2pc set
    std::string relicSetB;   // second 2pc set; unused when relicFourPiece
    std::string planarSet;   // Planar Sphere + Link Rope set (always 2pc)

    // Section 22 component inputs beyond gear.
    OtherStatBonuses otherBonuses;
    ManualBaseStats manualBase;
    int level = 80;          // Character level (attacker level, Sec 4)

    // Manual opt-in per conditional set effect (Q3): effectId -> active.
    // DEFAULT false and never inferred — the user must explicitly enable
    // each bonus. Missing key also means false.
    std::unordered_map<std::string, bool> setEffectActive;

    // Phase 2: major-trace opt-ins (slot "A2"/"A4"/"A6" -> active, default
    // false) and Light Cone passive opt-in (default false). Mechanics are
    // NOT auto-resolved (§29): the enabled set travels to the sim as
    // informational notes; numeric resolution comes later.
    std::unordered_map<std::string, bool> traceActive;
    bool lcPassiveActive = false;

    // Phase 3: Eidolon level (0..6, default 0). Selects boosted manual
    // scaling values data-driven via the eidolon skillLevels lists, and
    // annotates non-skill Eidolons as informational notes.
    int eidolonLevel = 0;

    // Manual damage tables ("#1[i]%" values, decimals: 2.60 = 260%).
    // Per engine action key (basic/skill/ult/fua/memosprite): the base
    // multiplier and the Eidolon-boosted multiplier. The boosted value
    // applies when eidolonLevel reaches the E-number whose skillLevels
    // list the action (data-driven, varies per character).
    struct ScalingEntry
    {
        double base = 0.0;    // 0 = fall back to engine multipliers
        double boosted = 0.0; // 0 = no boosted value entered
    };
    std::unordered_map<std::string, ScalingEntry> scalingTables;

    bool initialized = false;
};

using LoadoutStore = std::unordered_map<std::string, CharacterLoadout>;

// Formats a stat value for the editable text buffers: percent-numbers
// without the "%" sign ("43.2"), flat values raw ("705").
inline std::string formatStatValueText(double value, const std::string& key)
{
    if (isPercentStatKey(key))
    {
        // Trim float noise: round to 2 decimals, drop trailing zeros.
        double rounded = std::round(value * 100.0) / 100.0;
        std::string text = std::to_string(rounded);
        text.erase(text.find_last_not_of('0') + 1, std::string::npos);
        if (!text.empty() && text.back() == '.')
            text.pop_back();
        return text;
    }
    long long whole = static_cast<long long>(std::llround(value));
    return std::to_string(whole);
}

// Fills in sane defaults (first valid main stat per slot, 5-star max value)
// the first time a character's loadout is touched. Safe to call repeatedly.
inline void ensureLoadoutDefaults(CharacterLoadout& loadout)
{
    if (loadout.initialized)
        return;

    for (size_t i = 0; i < loadout.gear.size(); ++i)
    {
        GearSlot slot = static_cast<GearSlot>(i);
        const auto& options = mainStatOptions(slot);
        if (!options.empty())
        {
            loadout.gear[i].mainStat = options.front().key;
            loadout.gear[i].mainStatValueText =
                formatStatValueText(mainStatMaxValue(options.front().key),
                                    options.front().key);
        }
    }

    loadout.initialized = true;
}

// Resets a piece's main-stat value to the 5-star max after the player picks
// a new main stat. The value stays freely editable afterwards.
inline void resetMainStatValueDefault(GearPiece& piece)
{
    piece.mainStatValueText =
        formatStatValueText(mainStatMaxValue(piece.mainStat), piece.mainStat);
}

// Picks the next legal substat for a row: skips the piece's current main
// stat and any stat already used by one of the piece's other three rows,
// mirroring the game's "no duplicate substats on one piece" rule.
inline std::string nextSubstatOption(const GearPiece& piece, int rowIndex)
{
    const auto& pool = substatPool();
    size_t poolSize = pool.size();

    auto isUsedElsewhere = [&](const std::string& key) {
        for (int i = 0; i < 4; ++i)
        {
            if (i == rowIndex)
                continue;
            if (piece.substats[i].statKey == key)
                return true;
        }
        return false;
    };

    const std::string& current = piece.substats[rowIndex].statKey;
    size_t searchStart = 0;
    for (size_t i = 0; i < poolSize; ++i)
    {
        if (pool[i].key == current)
        {
            searchStart = i + 1;
            break;
        }
    }

    for (size_t step = 0; step < poolSize; ++step)
    {
        size_t idx = (searchStart + step) % poolSize;
        const std::string& key = pool[idx].key;
        if (key == piece.mainStat)
            continue;
        if (isUsedElsewhere(key))
            continue;
        return key;
    }

    return current; // no legal alternative — shouldn't happen with a 12-stat pool
}
