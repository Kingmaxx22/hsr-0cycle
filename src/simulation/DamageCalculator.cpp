#include "DamageCalculator.h"
#include <algorithm>
#include <cctype>
#include <cmath>

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

    // Apply Plight difficulty override: forces enemy to use Level 100 DEF value
    int effectiveLevel = config.isPlightDifficulty ? 100 : attackerLevel;

    // Calculate effective DEF
    // DEF = Base DEF x (100% + DEF% - (DEF Reduction + DEF Ignore)) + Flat DEF
    double defPercentTotal = 1.0 + config.enemyDEFPercent - (config.defReductionPercent + config.defIgnorePercent);
    double effectiveDEF = config.enemyBaseDEF * defPercentTotal + config.flatDEFReduction;

    // Apply Defense Shred: DEF_Value = Effective DEF x (1 - Shred%)
    // Shred is capped at 100% (shredPercent clamped to 0-1)
    double shred = std::clamp(config.shredPercent, 0.0, 1.0);
    double defValue = effectiveDEF * (1.0 - shred);

    // Clamp DEF to 0 minimum
    defValue = std::max(0.0, defValue);

    // Calculate DEF Mult = 100% - [DEF_Value / (DEF_Value + Attacker Level x 10 + 200)]
    double denominator = defValue + 200.0 + 10.0 * static_cast<double>(effectiveLevel);

    if (denominator <= 0.0) {
        return 1.0;
    }

    double defMult = 1.0 - (defValue / denominator);

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

DefenseShredComparison calculateDefenseShredComparison(
    double enemyTotalDEF,
    double shredPercent,
    double compareShredPercent,
    int attackerLevel,
    bool isPlightDifficulty) {

    // Current calculation
    int effectiveLevel = isPlightDifficulty ? 100 : attackerLevel;

    double currentShred = std::clamp(shredPercent, 0.0, 1.0);
    double currentDefValue = enemyTotalDEF * (1.0 - currentShred);
    currentDefValue = std::max(0.0, currentDefValue);

    double currentDenominator = currentDefValue + 200.0 + 10.0 * static_cast<double>(effectiveLevel);
    double currentMult = 1.0;
    if (currentDenominator > 0.0) {
        currentMult = 1.0 - (currentDefValue / currentDenominator);
    }

    // Compare calculation
    double compareShred = std::clamp(compareShredPercent, 0.0, 1.0);
    double compareDefValue = enemyTotalDEF * (1.0 - compareShred);
    compareDefValue = std::max(0.0, compareDefValue);

    int compareLevel = isPlightDifficulty ? 100 : attackerLevel;
    double compareDenominator = compareDefValue + 200.0 + 10.0 * static_cast<double>(compareLevel);
    double compareMult = 1.0;
    if (compareDenominator > 0.0) {
        compareMult = 1.0 - (compareDefValue / compareDenominator);
    }

    // Relative gain = (finalMulti2 / finalMulti1) - 1
    double relativeGain = (compareMult / currentMult) - 1.0;

    return { currentMult, compareMult, relativeGain };
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
    // Exact DB value wins when provided; otherwise fall back to buckets.
    double enemyBaseRES = 0.20; // Default Neutral = 20%

    if (config.explicitBaseRES >= 0.0) {
        enemyBaseRES = config.explicitBaseRES;
    } else {
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
    }

    // Clamp enemy RES between -100% and 90% before applying penetration
    double clampedRES = std::clamp(enemyBaseRES, -1.0, 0.9);

    // Apply RES penetration. PEN is NOT capped at 100%: values above 100%
    // are legal (practical max ~190% vs a 90% RES enemy) and drive
    // effective RES negative, handled by the [0.1, 2.0] mult clamp below.
    double effectiveRES = clampedRES - std::max(0.0, config.resPenetration);

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

ResistancePenComparison calculateResistancePenetrationComparison(
    double enemyRES,
    double sumPEN,
    double comparePEN) {

    // Current calculation (PEN uncapped above, see calculateResistanceMultiplier)
    double clampedRES = std::clamp(enemyRES, -1.0, 0.9);
    double effectiveRES = clampedRES - std::max(0.0, sumPEN);
    double currentMult = 1.0 - effectiveRES;
    currentMult = std::clamp(currentMult, 0.1, 2.0);

    // Compare calculation (PEN uncapped above, see calculateResistanceMultiplier)
    double compareClampedRES = std::clamp(enemyRES, -1.0, 0.9);
    double compareEffectiveRES = compareClampedRES - std::max(0.0, comparePEN);
    double compareMult = 1.0 - compareEffectiveRES;
    compareMult = std::clamp(compareMult, 0.1, 2.0);

    // Relative gain = (finalMulti2 / finalMulti1) - 1
    double relativeGain = (compareMult / currentMult) - 1.0;

    return { currentMult, compareMult, relativeGain };
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
// SECTION 15: PERCENTAGE SPEED CALCULATION
// ============================================================================
// Total Speed = Base Speed + (Base Speed x %Speed) + Flat Speed
//
// Rule: %Speed bonuses scale off BASE Speed only, NOT current/total Speed.
// This means two characters with different current Speed totals but the same
// Base Speed get an IDENTICAL flat Speed gain from the same %Speed buff.
//
// Special case: Characters whose kit or Light Cone directly raises BASE Speed
// (e.g. Aglaea's Light Cone) get proportionally more value from %Speed buffs
// than characters who only add Speed via substats/boots (since substats/boots
// add to the %Speed or Flat Speed terms, not Base Speed).

double calculateSpeedWithBaseBonus(double characterBase, double percentBonus, double flatBonus) {
    // Total Speed = Base Speed + (Base Speed x %Speed) + Flat Speed
    // %Speed is applied to characterBase (base speed) only
    return characterBase * (1.0 + percentBonus) + flatBonus;
}

// ============================================================================
// SECTION 16: SPEED BREAKPOINTS (INFORMATIONAL / WARNING LOGIC)
// ============================================================================
// Speed breakpoint warnings/disclaimers for UI display.
//
// Rules:
// - Common cited breakpoints (134, 143, etc.) are only accurate within a SINGLE
//   wave of combat.
// - In multi-wave content (most MoC stages), the team gets re-sorted by Speed at
//   the start of each new wave, resetting any Action Value lead — making most
//   breakpoints inaccurate across wave transitions.
// - 134 Speed is the one broadly reliable breakpoint (guarantees 2 actions in
//   the first wave). Its main value is for players attempting to 0-cycle.
// - Outside Pure Fiction (which has fixed, countable cycles), there is usually
//   no meaningful hard threshold — higher Speed is just generally better without
//   one specific target number.
//
// If implementing a "breakpoint calculator" UI feature, include this disclaimer
// text near the output rather than presenting breakpoints as universally reliable.

const char* getSpeedBreakpointDisclaimer() {
    return R"(
Speed Breakpoint Disclaimer:

Commonly cited speed breakpoints (e.g., 134 speed = guaranteed two actions,
143 speed, etc.) are ONLY accurate within a SINGLE wave of combat.

In multi-wave content (most Memory of Chaos stages), the action order gets
re-sorted by Speed at the start of each new wave, which resets any Action
Value lead that may have built up. This means breakpoints that are accurate
for wave 1 may be completely invalid for wave 2, 3, etc.

134 Speed is the one broadly reliable breakpoint — it guarantees 2 actions
in the first wave. Its primary value is for players attempting to 0-cycle
(start combat with 0 Action Progress).

Outside Pure Fiction (which has fixed, countable cycles), there is usually
no meaningful hard threshold. Higher Speed is generally better without one
specific target number.

If you see speed breakpoint recommendations, treat them as approximate
informational guides only, not guarantees — especially for multi-wave content.
)";
}

double calculateVulnerabilityMultiplier(const VulnerabilityConfig& config) {
    // Standard vulnerability: capped at 250% (3.5x), includes enemy self-vuln
    if (config.vulnType == EnemyVulnerabilityType::Special) {
        // Special enemy vuln: uncapped, 1 + specialVulnEnemy%
        return 1.0 + config.specialVulnEnemy;
    } else {
        // Standard vuln: 1 + sumVULN, capped at 3.5 (250%)
        double vulnMult = 1.0 + config.sumVULN;
        return std::min(vulnMult, 3.5);
    }
}

double calculateVulnerabilityMultiplier(double sumVULN, EnemyVulnerabilityType vulnType, double specialVulnEnemy) {
    VulnerabilityConfig config;
    config.sumVULN = sumVULN;
    config.vulnType = vulnType;
    config.specialVulnEnemy = specialVulnEnemy;

    return calculateVulnerabilityMultiplier(config);
}

double calculateVulnerabilityMultiplier(double sumVULN) {
    // Standard vuln with default type and no special enemy VULN
    return calculateVulnerabilityMultiplier(sumVULN, EnemyVulnerabilityType::Standard, 0.0);
}

// ============================================================================
// SECTION 13: EFFECT HIT RATE (EHR) / DEBUFF APPLICATION CHANCE
// ============================================================================
// Final Chance = Base Chance × (1 - Effect RES) × (1 + EHR)
//
// Rules:
// - Final Chance is capped at 100%.
// - Effect RES is capped at 100%.
// - EHR itself has no upper cap.
//
// For MULTI-HIT attacks, use Bernoulli Trials to compute the chance of AT LEAST
// ONE successful proc across all hits:
//
// At-Least-One Chance = 1 - (1 - Final Chance)^Hit Count
//
// IMPORTANT: "Hit Count" for debuff/DoT application must be manually configured
// per skill/effect, NOT auto-derived from the attack's hit count.

double calculateEffectHitRate(const EffectHitRateConfig& config) {
    // Final Chance = Base Chance × (1 - Effect RES) × (1 + EHR)
    double effectiveRES = std::clamp(config.effectRES, 0.0, 1.0);
    double finalChance = config.baseChance * (1.0 - effectiveRES) * (1.0 + config.ehr);

    // Cap at 100%
    return std::min(finalChance, 1.0);
}

double calculateEffectHitRate(double baseChance, double effectRES, double ehr, int hitCount) {
    EffectHitRateConfig config;
    config.baseChance = baseChance;
    config.effectRES = effectRES;
    config.ehr = ehr;
    config.hitCount = hitCount;

    return calculateEffectHitRate(config);
}

double calculateEffectHitRateAtLeastOne(double finalChancePerHit, int hitCount) {
    // At-Least-One Chance = 1 - (1 - Final Chance)^Hit Count
    // If hitCount is 0 or negative, return 0
    if (hitCount <= 0) {
        return 0.0;
    }

    double result = 1.0 - std::pow(1.0 - finalChancePerHit, hitCount);

    // Cap at 100%
    return std::min(result, 1.0);
}

double calculateEffectHitRateAtLeastOne(const EffectHitRateConfig& config) {
    return calculateEffectHitRateAtLeastOne(
        calculateEffectHitRate(config),
        config.hitCount
    );
}

// ============================================================================
// SECTION 14: PUNCHLINE / BANGER STACK MULTIPLIER
// ============================================================================
// DMG Multiplier = 1 + [(Stacks x 5) / (Stacks + 240)]
//
// Rules:
// - StackType::Punchline: Elation's Punchline stacks
// - StackType::Banger: Certified Banger stacks
// - These are SEPARATE pools, never combined (summing would be incorrect)
// - No hard cap, but strong diminishing returns as stacks grow (natural formula property)

double calculatePunchlineBangerMultiplier(const PunchlineBangerConfig& config) {
    // DMG Multiplier = 1 + [(Stacks x 5) / (Stacks + 240)]
    double stacks = static_cast<double>(config.stackCount);

    // Formula: 1 + (stacks * 5) / (stacks + 240)
    // Note: If stacks is 0, result is 1 + 0/240 = 1.0 (no bonus)
    double mult = 1.0 + (stacks * 5.0) / (stacks + 240.0);

    return mult;
}

double calculatePunchlineBangerMultiplier(int stackCount, StackType stackType) {
    PunchlineBangerConfig config;
    config.stackCount = stackCount;
    config.stackType = stackType;

    return calculatePunchlineBangerMultiplier(config);
}

double calculateCritMultiplier(double critRate, double critDmg) {
    // Expected-value convention: average damage scales by 1 + rate x dmg.
    // Rate is a chance and clamps to [0, 1]; crit DMG floors at 0.
    double rate = std::clamp(critRate, 0.0, 1.0);
    double dmg = std::max(0.0, critDmg);
    return 1.0 + rate * dmg;
}

namespace {
std::string lowerElement(const std::string& s)
{
    std::string out = s;
    for (char& ch : out)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return out;
}
} // namespace

double resolveRES(const std::map<std::string, double>& resMap,
                  const std::vector<std::string>& weaknesses,
                  const std::string& element) {
    std::string el = lowerElement(element);
    // Weakness always wins, even if the map also has the element.
    for (const auto& w : weaknesses) {
        if (lowerElement(w) == el)
            return 0.0;
    }
    for (const auto& kv : resMap) {
        if (lowerElement(kv.first) == el)
            return kv.second;
    }
    return 0.20;
}

double calculateHealAmount(double skillHealMultiplier, double outgoingHealingBoost) {
    return skillHealMultiplier * (1.0 + std::max(0.0, outgoingHealingBoost));
}

// Base Break DMG by attacker level (1-95).
// Sourced (spec-grade): HSR wiki Toughness page Level Multiplier table
// (https://honkai-star-rail.fandom.com/wiki/Toughness#Level_Multiplier),
// anchor-verified against the HSR DMG Calculator sheet (L65=2176.7983)
// and hsr-optimizer damageCalculator.ts (L80=3767.5533).
// Levels 81-95 are enemy-exclusive per the wiki (kept for completeness).
static double baseBreakByLevelTable(int level)
{
    static const double kTable[96] = {
        0.0, // [0] unused
        54, // [1]
        58, // [2]
        62, // [3]
        67.5264, // [4]
        70.5094, // [5]
        73.5228, // [6]
        76.566, // [7]
        79.6385, // [8]
        82.7395, // [9]
        85.8684, // [10]
        91.4944, // [11]
        97.068, // [12]
        102.5892, // [13]
        108.0579, // [14]
        113.4743, // [15]
        118.8383, // [16]
        124.1499, // [17]
        129.4091, // [18]
        134.6159, // [19]
        139.7703, // [20]
        149.3323, // [21]
        158.8011, // [22]
        168.1768, // [23]
        177.4594, // [24]
        186.6489, // [25]
        195.7452, // [26]
        204.7484, // [27]
        213.6585, // [28]
        222.4754, // [29]
        231.1992, // [30]
        246.4276, // [31]
        261.181, // [32]
        275.4733, // [33]
        289.3179, // [34]
        302.7275, // [35]
        315.7144, // [36]
        328.2905, // [37]
        340.4671, // [38]
        352.2554, // [39]
        363.6658, // [40]
        408.124, // [41]
        451.7883, // [42]
        494.6798, // [43]
        536.8188, // [44]
        578.2249, // [45]
        618.9172, // [46]
        658.9138, // [47]
        698.2325, // [48]
        736.8905, // [49]
        774.9041, // [50]
        871.0599, // [51]
        964.8705, // [52]
        1056.4206, // [53]
        1145.791, // [54]
        1233.0585, // [55]
        1318.2965, // [56]
        1401.575, // [57]
        1482.9608, // [58]
        1562.5178, // [59]
        1640.3068, // [60]
        1752.3215, // [61]
        1861.9011, // [62]
        1969.1242, // [63]
        2074.0659, // [64]
        2176.7983, // [65]
        2277.3904, // [66]
        2375.9085, // [67]
        2472.416, // [68]
        2566.9739, // [69]
        2659.6406, // [70]
        2780.3044, // [71]
        2898.6022, // [72]
        3014.6029, // [73]
        3128.3729, // [74]
        3239.9758, // [75]
        3349.473, // [76]
        3456.9236, // [77]
        3562.3843, // [78]
        3665.9099, // [79]
        3767.5533, // [80]
        3957.8618, // [81]
        4155.2118, // [82]
        4359.8638, // [83]
        4572.0878, // [84]
        4792.1641, // [85]
        5020.3833, // [86]
        5257.0466, // [87]
        5502.4664, // [88]
        5756.9667, // [89]
        6020.8836, // [90]
        6294.5654, // [91]
        6578.3734, // [92]
        6872.6823, // [93]
        7177.8806, // [94]
        7494.3713, // [95]
    };
    if (level < 1) return kTable[1];
    if (level > 95) return kTable[95];
    return kTable[level];
}

double baseBreakByLevel(int level) {
    return baseBreakByLevelTable(level);
}

double elementBreakMultiplier(const std::string& element) {
    std::string el = lowerElement(element);
    // Wiki Break Properties table (+ calculator sheet + two guides).
    if (el == "physical" || el == "fire") return 2.0;
    if (el == "wind") return 1.5;
    if (el == "ice" || el == "lightning") return 1.0;
    if (el == "quantum" || el == "imaginary") return 0.5;
    // Unknown/empty element: neutral default (documented, not sourced).
    return 1.0;
}

BreakDamageResult calculateBreakDamage(const BreakDamageConfig& config) {
    BreakDamageResult result;

    double base = baseBreakByLevel(config.attackerLevel);
    double elemMult = elementBreakMultiplier(config.attackerElement);
    result.baseBreak = elemMult * base;

    // Max Toughness Multiplier = 0.5 + maxToughness / 40 (wiki).
    double maxTough = std::max(0.0, config.enemyMaxToughness);
    result.toughnessMultiplier = 0.5 + maxTough / 40.0;

    result.defenseMultiplier = calculateDefenseMultiplier(config.defenseConfig);
    result.resistanceMultiplier = calculateResistanceMultiplier(config.resistanceConfig);
    result.vulnerabilityMultiplier = calculateVulnerabilityMultiplier(config.vulnerabilityConfig);
    result.universalReductionMultiplier =
        calculateUniversalDamageReductionMultiplier(config.universalReductionConfig);

    // Ability Multiplier defaults to 1.0 (unmodeled per-hit ability mults).
    // CRIT, DMG Boost and Weaken are excluded per source.
    result.finalBreakDamage = result.baseBreak
        * result.toughnessMultiplier
        * (1.0 + std::max(0.0, config.breakEffect))
        * (1.0 + std::max(0.0, config.breakDmgIncrease))
        * result.defenseMultiplier
        * result.resistanceMultiplier
        * result.vulnerabilityMultiplier
        * result.universalReductionMultiplier;

    return result;
}

double calculateSuperBreakDamage(
    double toughnessDamage,
    double breakEffect,
    double superBreakModifier,
    double defenseMultiplier,
    double resistanceMultiplier,
    double vulnerabilityMultiplier,
    double universalBrokenMultiplier) {
    if (toughnessDamage <= 0.0 || superBreakModifier <= 0.0)
        return 0.0;
    // (baseBreakByLevel(80) / 10) per hsr-optimizer SuperBreakDamageFunction.
    double superBreakBase = (baseBreakByLevel(80) / 10.0) * toughnessDamage;
    return superBreakBase
        * (1.0 + std::max(0.0, breakEffect))
        * superBreakModifier
        * defenseMultiplier
        * resistanceMultiplier
        * vulnerabilityMultiplier
        * universalBrokenMultiplier;
}

// ============================================================================
// SECTION 1: MASTER DAMAGE FORMULA IMPLEMENTATION
// ============================================================================
// Outgoing DMG = Base DMG x DMG% Mult x DEF Mult x RES Mult x DMG Taken Mult
//                x Universal DMG Reduction Mult x Weakeness Mult x Vulnerability Mult

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

    // Calculate Vulnerability Multiplier (Section 11)
    result.vulnerabilityMultiplier = calculateVulnerabilityMultiplier(config.vulnerabilityConfig);

    // Apply master formula:
    // Outgoing DMG = Base DMG x DMG% Mult x DEF Mult x RES Mult x DMG Taken Mult
    //                x Universal DMG Reduction Mult x Weakeness Mult x Vulnerability Mult
    result.finalDamage = result.baseDamage
                       * result.dmgPercentMultiplier
                       * result.defenseMultiplier
                       * result.resistanceMultiplier
                       * result.damageTakenMultiplier
                       * result.universalReductionMultiplier
                       * result.weakenessMultiplier
                       * result.vulnerabilityMultiplier;

    return result;
}

// ============================================================================
// SECTION 17: ACTION ADVANCE
// ============================================================================
// Action Advance reduces the remaining AV requirement by a percentage of
// the remaining requirement (not a flat amount).
//
// Rules:
// - Advance is normally capped at 100% of the CURRENT remaining requirement
//   (cannot push a turn to before 0 AV).
// - EXCEPTION: if a character has previously been "delayed" such that they
//   need MORE than 100% action to take their next turn (e.g. 120% required),
//   the effective advance cap rises to match that higher requirement (e.g. up
//   to 120%). This needs a delayedActionRequirement field that defaults to 100%.
// - Action Advance and Speed have an inverse value relationship: faster
//   characters have a smaller absolute AV pool remaining, so the same % Action
//   Advance yields a smaller absolute time savings for them. This means Action
//   Advance is worth MORE on slow characters and LESS on fast characters.
//
// Inputs: currentRemainingAVPercent (0.0 to delayedActionRequirement),
//         advancePercent (0.0 to 1.0, as decimal or percentage),
//         delayedActionRequirementPercent (defaults to 100.0, can be higher
//           if character was previously delayed)
// Output: newRemainingAVPercent (clamped such that it does not go below
//         delayedActionRequirementPercent - meaning the result cannot be
//         less than delayedActionRequirementPercent - currentRemainingAVPercent)
double calculateActionAdvance(
    double currentRemainingAVPercent,
    double advancePercent,
    double delayedActionRequirementPercent) {

    // Clamp advancePercent to valid range [0, 1]
    double advance = std::clamp(advancePercent, 0.0, 1.0);

    // Calculate remaining AV after advance
    // Advance clears 'advancePercent' of the REMAINING requirement
    double remainingAfterAdvance = currentRemainingAVPercent * (1.0 - advance);

    // The result cannot go below the delayedActionRequirement ceiling.
    // If delayedActionRequirementPercent <= 100.0, no special floor (normal behavior):
    if (delayedActionRequirementPercent <= 100.0) {
        // Normal case: just return the calculated remaining, but ensure it's not negative
        return std::max(0.0, remainingAfterAdvance);
    }

    // Delayed case: delayedActionRequirementPercent > 100.0
    // The character needs more than 100% AV. The floor is elevated:
    // If the requirement is 120%, at least 20% of the "extra" remains.
    // Clamp remainingAfterAdvance to not go below (delayed - 100).
    double floorValue = delayedActionRequirementPercent - 100.0;
    return std::max(remainingAfterAdvance, floorValue);
}

// ============================================================================
// SECTION 18: MID-TURN SPEED CHANGES
// ============================================================================
// When a character's Speed changes during an action, do not recalculate the
// entire action from zero. Account for action progress already spent.
//
// Conceptually:
//
// Raw AV at old Speed
// ↓
// AV already spent
// ↓
// Action progress
// ↓
// Apply advance if applicable
// ↓
// New Speed
// ↓
// Remaining AV using new Speed
//
// Formula:
//
// Raw AV at old Speed = 10000 / Old Speed
// % Action Taken = (AV already spent / Raw AV at old Speed) × 100 + Action Advance % already applied
// Remaining AV at new Speed = (1 - % Action Taken / 100) × (10000 / New Speed)
// Total AV for the turn = AV already spent + Remaining AV at new Speed
//
// Worked example:
//   oldSpeed = 102, avSpent = 45, actionAdvancePercent = 24, newSpeed = 137
//   Raw AV at old Speed = 10000 / 102 = ~98.04
//   % Action Taken = (45 / 98.04 × 100) + 24 = ~69.9%
//   Remaining AV at new Speed = (1 - 0.699) × (10000 / 137) = ~21.97
//   Total AV for the turn = 45 + 21.97 = ~66.97
//

double calculateMidTurnSpeedChange(
    double oldSpeed,
    double avAlreadySpent,
    double actionAdvancePercentAlreadyApplied,
    double newSpeed)
{
    // Raw AV at old Speed = 10000 / Old Speed
    double rawAVOldSpeed = 10000.0 / oldSpeed;

    // % Action Taken =
    // (AV already spent / Raw AV at old Speed) × 100
    // + Action Advance % already applied
    double percentActionTaken =
        (avAlreadySpent / rawAVOldSpeed) * 100.0
        + actionAdvancePercentAlreadyApplied;

    // Remaining AV at new Speed
    double remainingAVNewSpeed =
        (1.0 - (percentActionTaken / 100.0))
        * (10000.0 / newSpeed);

    // Total AV for the turn
    double totalAVForTurn =
        avAlreadySpent + remainingAVNewSpeed;

    return totalAVForTurn;
}

} // namespace damage
} // namespace hsr
