#include "DamageCalculator.h"
#include <algorithm>

namespace hsr {
namespace damage {

// ============================================================================
// SECTION 2: BASE DMG CALCULATION IMPLEMENTATION
// ============================================================================
// Base DMG = (Skill Multiplier + Extra Multiplier) x Scaling Attribute + Extra DMG

double calculateBaseDamage(const BaseDamageConfig& config) {
    return (config.skillMultiplier + config.extraMultiplier) * config.scalingAttributeValue + config.extraDMG;
}

double calculateBaseDamage(
    double skillMultiplier,
    double scalingAttributeValue,
    double extraMultiplier,
    double extraDMG) {
    
    return (skillMultiplier + extraMultiplier) * scalingAttributeValue + extraDMG;
}

// ============================================================================
// SECTION 3: DEF MULTIPLIER CALCULATION IMPLEMENTATION
// ============================================================================
// DEF Mult = (Attacker Level + 20) / [(Attacker Level + 20) + (Enemy DEF x (1 - DEF Ignore))]

double calculateDefenseMultiplier(const DefenseMultiplierConfig& config) {
    int attackerLevel = std::max(1, config.attackerLevel);
    int enemyDEF = std::max(0, config.enemyDEF);
    double defIgnore = std::clamp(config.defIgnore, 0.0, 1.0);
    
    double numerator = static_cast<double>(attackerLevel + 20);
    double effectiveEnemyDEF = enemyDEF * (1.0 - defIgnore);
    double denominator = numerator + effectiveEnemyDEF;
    
    if (denominator <= 0.0) {
        return 1.0;
    }
    
    return numerator / denominator;
}

double calculateDefenseMultiplier(
    int attackerLevel,
    int enemyDEF,
    double defIgnore) {
    
    DefenseMultiplierConfig config;
    config.attackerLevel = attackerLevel;
    config.enemyDEF = enemyDEF;
    config.defIgnore = defIgnore;
    
    return calculateDefenseMultiplier(config);
}

// ============================================================================
// SECTION 4: RESISTANCE MULTIPLIER CALCULATION IMPLEMENTATION
// ============================================================================
// RES Mult calculation based on enemy resistance and RES Penetration
//
// Effective RES = Enemy RES - RES Penetration - RES Reduction
// 
// If Effective RES >= 0:
//   RES Mult = 1 - Effective RES
// If Effective RES < 0:
//   RES Mult = 1 - (Effective RES / 2)
//   (Negative resistance gives diminished returns)

double calculateResistanceMultiplier(const ResistanceMultiplierConfig& config) {
    double enemyRES = config.enemyResistance;
    double resPen = std::clamp(config.resPenetration, 0.0, 1.0);
    double resRed = std::clamp(config.resReduction, 0.0, 1.0);
    
    // Calculate effective resistance after penetration and reduction
    double effectiveRES = enemyRES - resPen - resRed;
    
    // Apply resistance multiplier formula
    if (effectiveRES >= 0.0) {
        // Positive resistance reduces damage
        return 1.0 - effectiveRES;
    } else {
        // Negative resistance increases damage, but with diminished returns
        return 1.0 - (effectiveRES / 2.0);
    }
}

double calculateResistanceMultiplier(
    double enemyResistance,
    double resPenetration,
    double resReduction) {
    
    ResistanceMultiplierConfig config;
    config.enemyResistance = enemyResistance;
    config.resPenetration = resPenetration;
    config.resReduction = resReduction;
    
    return calculateResistanceMultiplier(config);
}

// ============================================================================
// SECTION 5: DMG TAKEN MULTIPLIER CALCULATION IMPLEMENTATION
// ============================================================================
// DMG Taken Mult = 1 + sum of all DMG Taken buffs on enemy

double calculateDamageTakenMultiplier(const DamageTakenConfig& config) {
    return 1.0 + config.dmgTakenBuff;
}

double calculateDamageTakenMultiplier(double dmgTakenBuff) {
    DamageTakenConfig config;
    config.dmgTakenBuff = dmgTakenBuff;
    
    return calculateDamageTakenMultiplier(config);
}

// ============================================================================
// SECTION 6: UNIVERSAL DMG REDUCTION CALCULATION IMPLEMENTATION
// ============================================================================
// Universal DMG Reduction Mult = 1 - sum of all universal damage reductions

double calculateUniversalDamageReductionMultiplier(const UniversalDamageReductionConfig& config) {
    double reduction = std::clamp(config.universalReduction, 0.0, 1.0);
    return 1.0 - reduction;
}

double calculateUniversalDamageReductionMultiplier(double universalReduction) {
    UniversalDamageReductionConfig config;
    config.universalReduction = universalReduction;
    
    return calculateUniversalDamageReductionMultiplier(config);
}

// ============================================================================
// SECTION 7: WEAKEN MULTIPLIER CALCULATION IMPLEMENTATION
// ============================================================================
// Weaken Mult = 1 - Weaken DMG Reduction

double calculateWeakenMultiplier(const WeakenConfig& config) {
    double weaken = std::clamp(config.weakenReduction, 0.0, 1.0);
    return 1.0 - weaken;
}

double calculateWeakenMultiplier(double weakenReduction) {
    WeakenConfig config;
    config.weakenReduction = weakenReduction;
    
    return calculateWeakenMultiplier(config);
}

// ============================================================================
// SECTION 1: MASTER DAMAGE FORMULA IMPLEMENTATION
// ============================================================================
// Outgoing DMG = Base DMG x DMG% Mult x DEF Mult x RES Mult x DMG Taken Mult
//                x Universal DMG Reduction Mult x Weaken Mult

DamageResult calculateOutgoingDamage(const MasterDamageConfig& config) {
    DamageResult result;
    
    // Calculate Base DMG (Section 2)
    result.baseDamage = calculateBaseDamage(config.baseDamageConfig);
    
    // Calculate DEF Multiplier (Section 3)
    result.defenseMultiplier = calculateDefenseMultiplier(config.defenseConfig);
    
    // Calculate RES Multiplier (Section 4)
    result.resistanceMultiplier = calculateResistanceMultiplier(config.resistanceConfig);
    
    // Calculate DMG Taken Multiplier (Section 5)
    result.damageTakenMultiplier = calculateDamageTakenMultiplier(config.damageTakenConfig);
    
    // Calculate Universal DMG Reduction Multiplier (Section 6)
    result.universalReductionMultiplier = calculateUniversalDamageReductionMultiplier(
        config.universalReductionConfig);
    
    // Calculate Weaken Multiplier (Section 7)
    result.weakenMultiplier = calculateWeakenMultiplier(config.weakenConfig);
    
    // Apply master formula:
    // Outgoing DMG = Base DMG x DMG% Mult x DEF Mult x RES Mult x DMG Taken Mult
    //                x Universal DMG Reduction Mult x Weaken Mult
    result.finalDamage = result.baseDamage
                       * config.dmgPercentMultiplier
                       * result.defenseMultiplier
                       * result.resistanceMultiplier
                       * result.damageTakenMultiplier
                       * result.universalReductionMultiplier
                       * result.weakenMultiplier;
    
    return result;
}

} // namespace damage
} // namespace hsr
