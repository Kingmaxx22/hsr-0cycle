#pragma once

#include "GearRules.h"

#include <array>
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
    std::array<SubstatRoll, 4> substats;
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

    bool initialized = false;
};

using LoadoutStore = std::unordered_map<std::string, CharacterLoadout>;

// Fills in sane defaults (first valid main stat per slot) the first time a
// character's loadout is touched. Safe to call repeatedly.
inline void ensureLoadoutDefaults(CharacterLoadout& loadout)
{
    if (loadout.initialized)
        return;

    for (size_t i = 0; i < loadout.gear.size(); ++i)
    {
        GearSlot slot = static_cast<GearSlot>(i);
        const auto& options = mainStatOptions(slot);
        if (!options.empty())
            loadout.gear[i].mainStat = options.front().key;
    }

    loadout.initialized = true;
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
