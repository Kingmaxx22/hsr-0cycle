#pragma once

// Section 22.3: automatic stat calculation.
//
// Character Base Stats -> Light Cone -> Relics/Planar (mains + substats)
//     -> Other Stat Bonuses -> Calculated Final Combat Stats -> Engine.
//
// This is the single source of truth (Sec 22.7): every consumer (damage
// calculator, simulation engine, AV, UI displays) reads the resolved
// CharacterConfig produced here. The player never re-enters a combined
// value by hand (Sec 22.6).
//
// Pure logic, no Raylib — safe to unit-test outside the UI.
//
// Set bonuses apply from the structured pieces."2"/"4".effects[] data.
// Unconditional stat_modifier effects auto-apply; conditioned effects need
// an explicit setEffectActive opt-in (default false, never inferred).

#include "CharacterDatabase.h"
#include "Characterloadout.h"
#include "LightConeDatabase.h"
#include "Relicsetdatabase.h"
#include "simulation/SimulationEngine.h"

#include <string>

namespace loadout {

// Bonus-only aggregates from gear mains + substats + set bonuses +
// other bonuses. Percent fields are decimals (0.15 = 15%); flats raw.
struct ResolvedBonuses
{
    double hpPct = 0.0;
    double atkPct = 0.0;
    double defPct = 0.0;
    double spdPct = 0.0; // %Spd scales base speed only (Sec 15)
    double flatHp = 0.0;
    double flatAtk = 0.0;
    double flatDef = 0.0;
    double flatSpd = 0.0;
    double critRate = 0.0;
    double critDmg = 0.0;
    double elemDmgPct = 0.0;
    double allTypeDmgPct = 0.0; // generic "damage" bonuses (Sec 3 pool)
    double resPen = 0.0;
    double ehr = 0.0;
    double effectRes = 0.0;
    double breakEffect = 0.0;
    double healingBoost = 0.0; // character.outgoingHealingBoost
    double energyRegen = 0.0;  // live: scales engine energy gain
    double breakDmgIncrease = 0.0; // live: x(1+increase) on break hits
};

// Final combat totals after the base+LC merge (Sec 9 formula stages).
struct ResolvedTotals
{
    double hp = 0.0;
    double atk = 0.0;
    double def = 0.0;
    double spd = 0.0;
};

// Parses one editable value buffer using the "%-number for % stats"
// convention shared with the relic editor. Unparseable/empty -> 0.
double parseStatValueText(const std::string& text, const std::string& key);

// Gear mains + substats + other bonuses, combined (no base stats, no LC).
ResolvedBonuses resolveGearBonuses(const CharacterLoadout& loadout);

// Stable opt-in key for one parsed set effect.
std::string setEffectId(const std::string& setId, const std::string& piece,
                        size_t index);

// Auto-derivation milestone: a damage_type condition whose element matches
// the attacker's element is active from turn state (no toggle needed).
// All other condition kinds stay manual-only (no stacks/HP/memosprite
// counters exist yet). Case-insensitive; empty element never auto-matches.
bool isSetEffectAutoActive(const SetEffectCondition& condition,
                           const std::string& attackerElement);

// Effective activation: manual opt-in OR auto-derivation.
bool isSetEffectActive(const SetEffectCondition& condition,
                       bool manualToggle, const std::string& attackerElement);

// Set bonuses from the equipped sets (Q1/Q3): unconditional stat_modifier
// effects always apply; conditioned effects apply ONLY when the matching
// setEffectActive toggle is true (default false, never inferred).
// 2pc: set matches relicSetA / relicSetB / planarSet.
// 4pc: relicFourPiece && set matches relicSetA.
ResolvedBonuses resolveSetBonuses(const CharacterLoadout& loadout,
                                  const RelicSetDatabase& relicSets,
                                  const std::string& attackerElement);

// Base stats: manual override wins when enabled (Sec 22.1/22.8),
// otherwise the character database entry.
void resolveBaseStats(const CharacterInfo& info, const CharacterLoadout& loadout,
                      double& hp, double& atk, double& def, double& spd);

// Light Cone base stats by equipped ID; zeros when none/invalid.
void resolveLightConeBase(const CharacterLoadout& loadout,
                          const LightConeDatabase& lightCones,
                          double& hp, double& atk, double& def);

// Full totals: (base + LC) x (1 + %) + flat, per Sec 9.
// Includes gear, set bonuses (gated conditionals need opt-in toggles),
// and other bonuses.
ResolvedTotals resolveTotals(const CharacterInfo& info,
                             const CharacterLoadout& loadout,
                             const LightConeDatabase& lightCones,
                             const RelicSetDatabase& relicSets);

// Full Sec 22.3 component workflow: fills every engine input the formula
// stages need (component splits for Sec 9, combat stats for Sec 21.2).
// Skill multipliers/rotation are per-character data (Sec 22 follow-up)
// and are intentionally left untouched for the caller to set.
void applyToCharacterConfig(hsr::CharacterConfig& config,
                            const CharacterInfo& info,
                            const CharacterLoadout& loadout,
                            const LightConeDatabase& lightCones,
                            const RelicSetDatabase& relicSets);

// Sec 22.4 completed-character workflow: entered finals straight into the
// engine config with manualStats set (mutually exclusive with 22.3).
// Percent args are decimals (0.70 = 70% crit rate).
void applyManualToCharacterConfig(hsr::CharacterConfig& config,
                                  const std::string& id,
                                  const std::string& name,
                                  double hp, double atk, double def, double spd,
                                  double critRate, double critDmg,
                                  double elemDmgPct, double resPen,
                                  double ehr, double effectRes);

} // namespace loadout
