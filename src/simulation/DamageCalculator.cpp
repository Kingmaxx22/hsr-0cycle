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
// SECTION 3: DMG% MULTIPLIER CALCULATION IMPLEMENTATION
// ============================================================================
// DMG% Mult = 100% + Elemental DMG% + All-Type DMG% + DoT DMG% + Other DMG%

double calculateDMGPercentMultiplier(const DMGPercentMultiplierConfig& config, bool includeDotDMG) {
    // Start with base 100% (1.0)
    double dmgPercentMult = 1.0;
    
    // Add Elemental DMG%
    dmgPercentMult += config.elementalDMG;
    
    // Add All-Type DMG%
    dmgPercentMult += config.allTypeDMG;
    
    // Add DoT DMG% only if calculating DoT damage
    if (includeDotDMG) {
        dmgPercentMult += config.dotDMG;
    }
    
    // Add active conditional buffs from otherDMG array
    for (const auto& buff : config.otherDMG) {
        if (buff.isActive) {
            dmgPercentMult += buff.dmgPercent;
        }
    }
    
    return dmgPercentMult;
}

double calculateDMGPercentMultiplier(
    double elementalDMG,
    double allTypeDMG,
    double dotDMG,
    const std::vector<ConditionalDMGBuff>& otherDMG,
    bool includeDotDMG) {
    
    DMGPercentMultiplierConfig config;
    config.elementalDMG = elementalDMG;
    config.allTypeDMG = allTypeDMG;
    config.dotDMG = dotDMG;
    config.otherDMG = otherDMG;
    
    return calculateDMGPercentMultiplier(config, includeDotDMG);
}

// ============================================================================
// SECTION 4: DEF MULTIPLIER CALCULATION IMPLEMENTATION
// ============================================================================
// DEF = Base DEF x (100% + DEF% - (DEF Reduction + DEF Ignore)) + Flat DEF
// DEF Mult = 100% - [DEF / (DEF + 200 + 10 x Attacker Level)]
// Rule: DEF cannot go below 0 (clamp to 0 minimum)

double calculateDefenseMultiplier(const DefenseMultiplierConfig& config) {
    int attackerLevel = std::max(1, config.attackerLevel);
    
    // Calculate effective DEF
    // DEF = Base DEF x (100% + DEF% - (DEF Reduction + DEF Ignore)) + Flat DEF
    double defPercentTotal = 1.0 + config.enemyDEFPercent - (config.defReductionPercent + config.defIgnorePercent);
    double effectiveDEF = config.enemyBaseDEF * defPercentTotal + config.flatDEFReduction;
    
    // Clamp DEF to 0 minimum
    effectiveDEF = std::max(0.0, effectiveDEF);
    
    // Calculate DEF Mult = 100% - [DEF / (DEF + 200 + 10 x Attacker Level)]
    double denominator = effectiveDEF + 200.0 + 10.0 * static_cast<double>(attackerLevel);
    
    if (denominator <= 0.0) {
        return 1.0;
    }
    
    double defMult = 1.0 - (effectiveDEF / denominator);
    
    return defMult;
}

double calculateDefenseMultiplier(
    int attackerLevel,
    double enemyBaseDEF,
    double enemyDEFPercent,
    double defReductionPercent,
    double defIgnorePercent,
    double flatDEFReduction) {
    
    DefenseMultiplierConfig config;
    config.attackerLevel = attackerLevel;
    config.enemyBaseDEF = enemyBaseDEF;
    config.enemyDEFPercent = enemyDEFPercent;
    config.defReductionPercent = defReductionPercent;
    config.defIgnorePercent = defIgnorePercent;
    config.flatDEFReduction = flatDEFReduction;
    
    return calculateDefenseMultiplier(config);
}

// ============================================================================
// SECTION 5: RESISTANCE MULTIPLIER CALCULATION IMPLEMENTATION
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
// SECTION 6: DMG TAKEN MULTIPLIER CALCULATION IMPLEMENTATION
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
// SECTION 7: UNIVERSAL DMG REDUCTION CALCULATION IMPLEMENTATION
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
// SECTION 8: WEAKEN MULTIPLIER CALCULATION IMPLEMENTATION
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
    
    // Store DMG% Multiplier
    result.dmgPercentMultiplier = config.dmgPercentMultiplier;
    
    // Calculate DEF Multiplier (Section 4)
    result.defenseMultiplier = calculateDefenseMultiplier(config.defenseConfig);
    
    // Calculate RES Multiplier (Section 5)
    result.resistanceMultiplier = calculateResistanceMultiplier(config.resistanceConfig);
    
    // Calculate DMG Taken Multiplier (Section 6)
    result.damageTakenMultiplier = calculateDamageTakenMultiplier(config.damageTakenConfig);
    
    // Calculate Universal DMG Reduction Multiplier (Section 7)
    result.universalReductionMultiplier = calculateUniversalDamageReductionMultiplier(
        config.universalReductionConfig);
    
    // Calculate Weaken Multiplier (Section 8)
    result.weakenMultiplier = calculateWeakenMultiplier(config.weakenConfig);
    
    // Apply master formula:
    // Outgoing DMG = Base DMG x DMG% Mult x DEF Mult x RES Mult x DMG Taken Mult
    //                x Universal DMG Reduction Mult x Weaken Mult
    result.finalDamage = result.baseDamage
                       * result.dmgPercentMultiplier
                       * result.defenseMultiplier
                       * result.resistanceMultiplier
                       * result.damageTakenMultiplier
                       * result.universalReductionMultiplier
                       * result.weakenMultiplier;
    
    return result;
}

} // namespace damage
} // namespace hsr
