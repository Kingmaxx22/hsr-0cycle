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
// RES Mult = 100% - (RES% - RES PEN%)
//
// Rules:
// - Base enemy RES to all elements = 20%, UNLESS the enemy has an innate weakness or resistance.
// - Enemy weak to the element -> RES = 0%.
// - Enemy resistant to the element -> RES = 40%.
// - RES is clamped between -100% (min) and 90% (max) before applying PEN.
//
// Effective RES = clamp(enemyBaseRES, -1.0, 0.9) - resPenetration
// RES Mult = 1 - Effective RES, clamped to range [0.1, 2.0]

double calculateResistanceMultiplier(const ResistanceMultiplierConfig& config) {
    // Determine base RES based on resistance type
    double enemyBaseRES = 0.20; // Default Neutral = 20%
    
    switch (config.resistanceType) {
        case EnemyResistanceType::Weak:
            enemyBaseRES = 0.0;   // Weak = 0%
            break;
        case EnemyResistanceType::Resistant:
            enemyBaseRES = 0.40;  // Resistant = 40%
            break;
        case EnemyResistanceType::Neutral:
        default:
            enemyBaseRES = 0.20;  // Neutral = 20%
            break;
    }
    
    // Clamp enemy RES between -100% and 90% before applying penetration
    double clampedRES = std::clamp(enemyBaseRES, -1.0, 0.9);
    
    // Apply RES penetration
    double effectiveRES = clampedRES - std::clamp(config.resPenetration, 0.0, 1.0);
    
    // Calculate RES Mult = 1 - Effective RES
    double resMult = 1.0 - effectiveRES;
    
    // Clamp final multiplier to range [0.1, 2.0]
    return std::clamp(resMult, 0.1, 2.0);
}

double calculateResistanceMultiplier(
    double enemyBaseRES,
    double resPenetration) {
    
    ResistanceMultiplierConfig config;
    config.resPenetration = resPenetration;
    
    // Set resistance type based on explicit base RES value
    if (enemyBaseRES <= 0.0) {
        config.resistanceType = EnemyResistanceType::Weak;
    } else if (enemyBaseRES >= 0.40) {
        config.resistanceType = EnemyResistanceType::Resistant;
    } else {
        config.resistanceType = EnemyResistanceType::Neutral;
    }
    
    return calculateResistanceMultiplier(config);
}

double calculateResistanceMultiplier(
    EnemyResistanceType resistanceType,
    double resPenetration) {
    
    ResistanceMultiplierConfig config;
    config.resistanceType = resistanceType;
    config.resPenetration = resPenetration;
    
    return calculateResistanceMultiplier(config);
}

// ============================================================================
// SECTION 6: DMG TAKEN MULTIPLIER CALCULATION IMPLEMENTATION
// ============================================================================
// DMG Taken Mult = 100% + Elemental DMG Taken% + All-Type DMG Taken%

double calculateDamageTakenMultiplier(const DamageTakenConfig& config) {
    // DMG Taken Mult = 1 + Elemental DMG Taken% + All-Type DMG Taken%
    return 1.0 + config.elementalDMGTaken + config.allTypeDMGTaken;
}

double calculateDamageTakenMultiplier(
    double elementalDMGTaken,
    double allTypeDMGTaken) {
    
    DamageTakenConfig config;
    config.elementalDMGTaken = elementalDMGTaken;
    config.allTypeDMGTaken = allTypeDMGTaken;
    
    return calculateDamageTakenMultiplier(config);
}

// ============================================================================
// SECTION 7: UNIVERSAL DMG REDUCTION MULTIPLIER IMPLEMENTATION
// ============================================================================
// Universal DMG Reduction Mult = 100% x (1 - Reduction_1) x (1 - Reduction_2) x ...
//
// Rules:
// - Multiple reduction sources stack MULTIPLICATIVELY
// - Unbroken enemies with Toughness apply a built-in 10% reduction (0.90 multiplier)
// - Broken enemies have no built-in Toughness reduction (1.00 multiplier)

double calculateUniversalDamageReductionMultiplier(const UniversalDamageReductionConfig& config) {
    // Start with base 100% (1.0)
    double universalReductionMult = 1.0;
    
    // If enemy is not broken, apply built-in Toughness reduction (10%)
    if (!config.isEnemyBroken) {
        universalReductionMult *= 0.90;  // 100% - 10% = 90% = 0.90
    }
    
    // Apply each reduction source multiplicatively
    for (double reduction : config.reductionSources) {
        // Clamp individual reduction to valid range [0, 1]
        double clampedReduction = std::clamp(reduction, 0.0, 1.0);
        universalReductionMult *= (1.0 - clampedReduction);
    }
    
    return universalReductionMult;
}

double calculateUniversalDamageReductionMultiplier(
    double universalReduction,
    bool isEnemyBroken) {
    
    UniversalDamageReductionConfig config;
    config.isEnemyBroken = isEnemyBroken;
    if (universalReduction > 0.0) {
        config.reductionSources.push_back(universalReduction);
    }
    
    return calculateUniversalDamageReductionMultiplier(config);
}

// ============================================================================
// SECTION 8: WEAKENESS MULTIPLIER IMPLEMENTATION
// ============================================================================
// Weakeness Mult = 100% - Weakeness%
//
// Only relevant when calculating damage dealt BY enemies (e.g. Natasha's or
// Sampo's passive Weaken effect reducing incoming enemy damage). Default 0%
// (mult = 1.0) for player-character outgoing damage calculations.

double calculateWeakenessMultiplier(const WeakenessConfig& config) {
    // Clamp weakeness to valid range [0, 1]
    double weakeness = std::clamp(config.weakenessPercent, 0.0, 1.0);
    return 1.0 - weakeness;
}

double calculateWeakenessMultiplier(double weakenessPercent) {
    WeakenessConfig config;
    config.weakenessPercent = weakenessPercent;
    
    return calculateWeakenessMultiplier(config);
}

// ============================================================================
// SECTION 9: CORE STAT TOTALS IMPLEMENTATION
// ============================================================================
// HP Total    = (Character Base HP  + LC Base HP)  x (1 + HP%)  + Flat HP
// ATK Total   = (Character Base ATK + LC Base ATK) x (1 + ATK%) + Flat ATK
// DEF Total   = (Character Base DEF + LC Base DEF) x (1 + DEF%) + Flat DEF
// Speed Total = Character Base Speed x (1 + Speed%) + Flat Speed
//
// Important rule: Light Cone BASE stats merge with character BASE stats FIRST,
// before percentage bonuses are applied.

double calculateTotalHP(const CoreStatConfig& config) {
    // HP Total = (Character Base HP + LC Base HP) x (1 + HP%) + Flat HP
    double combinedBase = config.characterBase + config.lightConeBase;
    return combinedBase * (1.0 + config.percentBonus) + config.flatBonus;
}

double calculateTotalATK(const CoreStatConfig& config) {
    // ATK Total = (Character Base ATK + LC Base ATK) x (1 + ATK%) + Flat ATK
    double combinedBase = config.characterBase + config.lightConeBase;
    return combinedBase * (1.0 + config.percentBonus) + config.flatBonus;
}

double calculateTotalDEF(const CoreStatConfig& config) {
    // DEF Total = (Character Base DEF + LC Base DEF) x (1 + DEF%) + Flat DEF
    double combinedBase = config.characterBase + config.lightConeBase;
    return combinedBase * (1.0 + config.percentBonus) + config.flatBonus;
}

double calculateTotalSpeed(const CoreStatConfig& config) {
    // Speed Total = Character Base Speed x (1 + Speed%) + Flat Speed
    // Note: Light Cones do not provide base Speed, so lightConeBase should be 0
    return config.characterBase * (1.0 + config.percentBonus) + config.flatBonus;
}

// Convenience overloads with individual parameters
double calculateTotalHP(double characterBase, double lightConeBase, double percentBonus, double flatBonus) {
    CoreStatConfig config;
    config.characterBase = characterBase;
    config.lightConeBase = lightConeBase;
    config.percentBonus = percentBonus;
    config.flatBonus = flatBonus;
    return calculateTotalHP(config);
}

double calculateTotalATK(double characterBase, double lightConeBase, double percentBonus, double flatBonus) {
    CoreStatConfig config;
    config.characterBase = characterBase;
    config.lightConeBase = lightConeBase;
    config.percentBonus = percentBonus;
    config.flatBonus = flatBonus;
    return calculateTotalATK(config);
}

double calculateTotalDEF(double characterBase, double lightConeBase, double percentBonus, double flatBonus) {
    CoreStatConfig config;
    config.characterBase = characterBase;
    config.lightConeBase = lightConeBase;
    config.percentBonus = percentBonus;
    config.flatBonus = flatBonus;
    return calculateTotalDEF(config);
}

double calculateTotalSpeed(double characterBase, double percentBonus, double flatBonus) {
    CoreStatConfig config;
    config.characterBase = characterBase;
    config.lightConeBase = 0.0;  // Light Cones don't provide base Speed
    config.percentBonus = percentBonus;
    config.flatBonus = flatBonus;
    return calculateTotalSpeed(config);
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
    
    // Calculate Weakeness Multiplier (Section 8)
    result.weakenessMultiplier = calculateWeakenessMultiplier(config.weakenessConfig);
    
    // Apply master formula:
    // Outgoing DMG = Base DMG x DMG% Mult x DEF Mult x RES Mult x DMG Taken Mult
    //                x Universal DMG Reduction Mult x Weakeness Mult
    result.finalDamage = result.baseDamage
                       * result.dmgPercentMultiplier
                       * result.defenseMultiplier
                       * result.resistanceMultiplier
                       * result.damageTakenMultiplier
                       * result.universalReductionMultiplier
                       * result.weakenessMultiplier;
    
    return result;
}

} // namespace damage
} // namespace hsr
