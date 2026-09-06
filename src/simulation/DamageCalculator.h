#ifndef DAMAGE_CALCULATOR_H
#define DAMAGE_CALCULATOR_H

#include <string>

namespace hsr {
namespace damage {

// ============================================================================
// SECTION 2: BASE DMG CALCULATION
// ============================================================================
// Base DMG = (Skill Multiplier + Extra Multiplier) x Scaling Attribute + Extra DMG
//
// Where:
// - Skill Multiplier = the % value from the skill description 
//   (e.g. "Deal DMG equal to 60% of ATK" -> 0.60)
// - Extra Multiplier = optional extra scaling some skills have 
//   (e.g. bonus DMG vs slowed enemies). Default 0 if not applicable.
// - Scaling Attribute = the stat the skill scales off 
//   (usually Total ATK, but can be HP/DEF for some characters)
// - Extra DMG = optional flat bonus damage some skills add. 
//   Default 0 if not applicable.

struct BaseDamageConfig {
    double skillMultiplier;      // Required: the skill's scaling multiplier (e.g., 0.60 for 60%)
    double extraMultiplier;      // Optional: additional multiplier (default 0)
    double scalingAttributeValue; // Required: the attribute value (ATK/HP/DEF)
    double extraDMG;             // Optional: flat bonus damage (default 0)
    
    BaseDamageConfig() 
        : skillMultiplier(0.0)
        , extraMultiplier(0.0)
        , scalingAttributeValue(0.0)
        , extraDMG(0.0) {}
};

/**
 * Calculates the base damage before multipliers.
 * 
 * @param config The base damage configuration containing multipliers and attributes
 * @return The calculated base damage value
 */
double calculateBaseDamage(const BaseDamageConfig& config);

// Convenience overload with individual parameters
double calculateBaseDamage(
    double skillMultiplier,
    double scalingAttributeValue,
    double extraMultiplier = 0.0,
    double extraDMG = 0.0
);

// ============================================================================
// SECTION 3: DEF MULTIPLIER CALCULATION
// ============================================================================
// DEF Mult = (Attacker Level + 20) / [(Attacker Level + 20) + (Enemy DEF x (1 - DEF Ignore))]
//
// This will be implemented in the next section.

struct DefenseMultiplierConfig {
    int attackerLevel;
    int enemyDEF;
    double defIgnore;  // DEF Ignore as decimal (e.g., 0.20 for 20%)
    
    DefenseMultiplierConfig()
        : attackerLevel(80)  // Default max level
        , enemyDEF(0)
        , defIgnore(0.0) {}
};

double calculateDefenseMultiplier(const DefenseMultiplierConfig& config);

// Convenience overload
double calculateDefenseMultiplier(
    int attackerLevel,
    int enemyDEF,
    double defIgnore = 0.0
);

// ============================================================================
// SECTION 4: RESISTANCE MULTIPLIER CALCULATION
// ============================================================================
// RES Mult calculation based on enemy resistance and RES Penetration
//
// This will be implemented in the next section.

struct ResistanceMultiplierConfig {
    double enemyResistance;  // Enemy's base RES (e.g., 0.10 for 10%)
    double resPenetration;   // Character's RES Penetration (e.g., 0.20 for 20%)
    double resReduction;     // RES Reduction debuffs on enemy (e.g., 0.10 for 10%)
    
    ResistanceMultiplierConfig()
        : enemyResistance(0.10)  // Default 10% resistance
        , resPenetration(0.0)
        , resReduction(0.0) {}
};

double calculateResistanceMultiplier(const ResistanceMultiplierConfig& config);

// Convenience overload
double calculateResistanceMultiplier(
    double enemyResistance,
    double resPenetration = 0.0,
    double resReduction = 0.0
);

// ============================================================================
// SECTION 5: DMG TAKEN MULTIPLIER CALCULATION
// ============================================================================
// DMG Taken Mult = 1 + sum of all DMG Taken buffs on enemy
//
// This includes effects like "Target takes X% more DMG" debuffs.

struct DamageTakenConfig {
    double dmgTakenBuff;  // Total DMG Taken buff as decimal (e.g., 0.20 for 20%)
    
    DamageTakenConfig()
        : dmgTakenBuff(0.0) {}
};

double calculateDamageTakenMultiplier(const DamageTakenConfig& config);

// Convenience overload
double calculateDamageTakenMultiplier(double dmgTakenBuff = 0.0);

// ============================================================================
// SECTION 6: UNIVERSAL DMG REDUCTION CALCULATION
// ============================================================================
// Universal DMG Reduction Mult = 1 - sum of all universal damage reductions
//
// This includes effects that reduce outgoing damage universally.

struct UniversalDamageReductionConfig {
    double universalReduction;  // Total universal reduction as decimal (e.g., 0.10 for 10%)
    
    UniversalDamageReductionConfig()
        : universalReduction(0.0) {}
};

double calculateUniversalDamageReductionMultiplier(const UniversalDamageReductionConfig& config);

// Convenience overload
double calculateUniversalDamageReductionMultiplier(double universalReduction = 0.0);

// ============================================================================
// SECTION 7: WEAKEN MULTIPLIER CALCULATION
// ============================================================================
// Weaken Mult = 1 - Weaken DMG Reduction
//
// Weaken is a specific debuff that reduces enemy's damage output,
// but can also affect damage dealt to weakened enemies in some contexts.

struct WeakenConfig {
    double weakenReduction;  // Weaken DMG reduction as decimal (e.g., 0.15 for 15%)
    
    WeakenConfig()
        : weakenReduction(0.0) {}
};

double calculateWeakenMultiplier(const WeakenConfig& config);

// Convenience overload
double calculateWeakenMultiplier(double weakenReduction = 0.0);

// ============================================================================
// SECTION 1: MASTER DAMAGE FORMULA
// ============================================================================
// Outgoing DMG = Base DMG x DMG% Mult x DEF Mult x RES Mult x DMG Taken Mult 
//                x Universal DMG Reduction Mult x Weaken Mult

struct MasterDamageConfig {
    // Base Damage components
    BaseDamageConfig baseDamageConfig;
    
    // DMG% Multiplier (includes elemental/type DMG bonuses, crit DMG, etc.)
    double dmgPercentMultiplier;  // e.g., 1.50 for 50% bonus
    
    // Other multipliers
    DefenseMultiplierConfig defenseConfig;
    ResistanceMultiplierConfig resistanceConfig;
    DamageTakenConfig damageTakenConfig;
    UniversalDamageReductionConfig universalReductionConfig;
    WeakenConfig weakenConfig;
    
    MasterDamageConfig()
        : dmgPercentMultiplier(1.0) {}
};

struct DamageResult {
    double baseDamage;
    double defenseMultiplier;
    double resistanceMultiplier;
    double damageTakenMultiplier;
    double universalReductionMultiplier;
    double weakenMultiplier;
    double finalDamage;
    
    DamageResult()
        : baseDamage(0.0)
        , defenseMultiplier(1.0)
        , resistanceMultiplier(1.0)
        , damageTakenMultiplier(1.0)
        , universalReductionMultiplier(1.0)
        , weakenMultiplier(1.0)
        , finalDamage(0.0) {}
};

/**
 * Calculates the full outgoing damage using the master formula.
 * 
 * Outgoing DMG = Base DMG x DMG% Mult x DEF Mult x RES Mult x DMG Taken Mult 
 *                x Universal DMG Reduction Mult x Weaken Mult
 * 
 * @param config The complete damage configuration
 * @return DamageResult containing all intermediate values and final damage
 */
DamageResult calculateOutgoingDamage(const MasterDamageConfig& config);

} // namespace damage
} // namespace hsr

#endif // DAMAGE_CALCULATOR_H
