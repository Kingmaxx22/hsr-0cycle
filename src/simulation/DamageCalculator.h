#ifndef DAMAGE_CALCULATOR_H
#define DAMAGE_CALCULATOR_H

#include <string>
#include <vector>

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
// SECTION 3: DMG% MULTIPLIER CALCULATION
// ============================================================================
// DMG% Mult = 100% + Elemental DMG% + All-Type DMG% + DoT DMG% + Other DMG%
//
// Rule: only sum the terms that are actually active/relevant for the current
// calculation context (e.g. a conditional buff like "+20% DMG vs Burning enemies"
// should only be included if that condition is currently true).

struct ConditionalDMGBuff {
    double dmgPercent;     // DMG% bonus as decimal (e.g., 0.20 for 20%)
    bool isActive;         // Whether this conditional buff is currently active
    
    ConditionalDMGBuff()
        : dmgPercent(0.0)
        , isActive(false) {}
    
    ConditionalDMGBuff(double percent, bool active)
        : dmgPercent(percent)
        , isActive(active) {}
};

struct DMGPercentMultiplierConfig {
    double elementalDMG;       // Elemental DMG% bonus (e.g., Fire DMG Boost)
    double allTypeDMG;         // All-Type DMG% bonus
    double dotDMG;             // DoT DMG% bonus (only used when calculating DoT)
    std::vector<ConditionalDMGBuff> otherDMG;  // Array of conditional buffs
    
    DMGPercentMultiplierConfig()
        : elementalDMG(0.0)
        , allTypeDMG(0.0)
        , dotDMG(0.0) {}
};

/**
 * Calculates the DMG% multiplier.
 * 
 * DMG% Mult = 100% + Elemental DMG% + All-Type DMG% + DoT DMG% + Other DMG%
 * Only active conditional buffs are included.
 * 
 * @param config The DMG% multiplier configuration
 * @param includeDotDMG Whether to include DoT DMG% (true if calculating DoT damage)
 * @return The calculated DMG% multiplier as a decimal (e.g., 1.359 for 35.9% bonus)
 */
double calculateDMGPercentMultiplier(const DMGPercentMultiplierConfig& config, bool includeDotDMG = false);

// Convenience overload
double calculateDMGPercentMultiplier(
    double elementalDMG,
    double allTypeDMG,
    double dotDMG,
    const std::vector<ConditionalDMGBuff>& otherDMG,
    bool includeDotDMG = false
);

// ============================================================================
// SECTION 4: DEF MULTIPLIER CALCULATION
// ============================================================================
// DEF Mult = 100% - [DEF / (DEF + 200 + 10 x Attacker Level)]
//
// DEF = Base DEF x (100% + DEF% - (DEF Reduction + DEF Ignore)) + Flat DEF
//
// Rule: DEF cannot go below 0 (clamp to 0 minimum before plugging into DEF Mult formula).

struct DefenseMultiplierConfig {
    // Attacker info
    int attackerLevel;
    
    // Enemy base DEF
    double enemyBaseDEF;
    
    // Enemy's own DEF buffs (usually 0 for enemies)
    double enemyDEFPercent;
    
    // Debuffs on enemy
    double defReductionPercent;  // DEF Reduction% from debuffs
    double defIgnorePercent;     // DEF Ignore% from character effects
    
    // Flat DEF reduction (rare)
    double flatDEFReduction;
    
    // Defense Shred% from debuffs (e.g., Shamans' S3, Welt's S2)
    // Clamped to 0-1 (0% to 100%), applies before DEF multiplier calculation
    double shredPercent;
    
    // If true, enemy uses Level 100 DEF value instead of actual level (Plight difficulty)
    bool isPlightDifficulty;
    
    DefenseMultiplierConfig()
        : attackerLevel(80)      // Default max level
        , enemyBaseDEF(0.0)
        , enemyDEFPercent(0.0)
        , defReductionPercent(0.0)
        , defIgnorePercent(0.0)
        , flatDEFReduction(0.0)
        , shredPercent(0.0)
        , isPlightDifficulty(false) {}
};

/**
 * Calculates the Defense multiplier.
 * 
 * DEF = Base DEF x (100% + DEF% - (DEF Reduction + DEF Ignore)) + Flat DEF
 * DEF Mult = 100% - [DEF / (DEF + 200 + 10 x Attacker Level)]
 * 
 * DEF is clamped to 0 minimum before calculating the multiplier.
 * 
 * @param config The defense multiplier configuration
 * @return The calculated DEF multiplier as a decimal
 */
double calculateDefenseMultiplier(const DefenseMultiplierConfig& config);

// Convenience overload
double calculateDefenseMultiplier(
    int attackerLevel,
    double enemyBaseDEF,
    double enemyDEFPercent = 0.0,
    double defReductionPercent = 0.0,
    double defIgnorePercent = 0.0,
    double flatDEFReduction = 0.0
);

// ============================================================================
// SECTION 5: RESISTANCE MULTIPLIER CALCULATION
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

enum class EnemyResistanceType {
    Neutral,    // Base RES = 20%
    Weak,       // Base RES = 0% (enemy weak to this element)
    Resistant   // Base RES = 40% (enemy resistant to this element)
};

enum class EnemyVulnerabilityType {
    Standard,   // Standard vuln, capped at 250% (3.5x) including enemy self-vuln
    Special     // Special enemy vuln, uncapped (only for Doomsday Beat and Sunday boss)
};

struct ResistanceMultiplierConfig {
    EnemyResistanceType resistanceType;  // Determines base RES value
    double resPenetration;               // Character's RES Penetration (e.g., 0.20 for 20%)
    // Exact base RES override (Sec 21.3: enemy DB values). >= 0 wins over
    // the type bucket; < 0 (default) keeps bucket behavior.
    double explicitBaseRES;

    ResistanceMultiplierConfig()
        : resistanceType(EnemyResistanceType::Neutral)  // Default 20% base RES
        , resPenetration(0.0)
        , explicitBaseRES(-1.0) {}
};

struct VulnerabilityConfig {
    double sumVULN;          // Sum of all VULN% (player + enemy self-vuln), clamped to 250% max
    EnemyVulnerabilityType vulnType;  // Standard or Special
    double specialVulnEnemy; // Special enemy VULN% (uncapped, only for 2 specific enemies)
    
    VulnerabilityConfig()
        : sumVULN(0.0)
        , vulnType(EnemyVulnerabilityType::Standard)
        , specialVulnEnemy(0.0) {}
};

/**
 * Configuration for Effect Hit Rate calculation.
 * 
 * Final Chance = Base Chance × (1 - Effect RES) × (1 + EHR)
 * 
 * Rules:
 * - Final Chance is capped at 100%.
 * - Effect RES is capped at 100%.
 * - EHR itself has no upper cap.
 * 
 * For MULTI-HIT attacks, use Bernoulli Trials to compute the chance of AT LEAST
 * ONE successful proc across all hits:
 * 
 * At-Least-One Chance = 1 - (1 - Final Chance)^Hit Count
 * 
 * IMPORTANT: "Hit Count" for debuff/DoT application is NOT simply the number of
 * hits in an attack. It must be manually configured per skill/effect, since many
 * attacks only attempt to apply a debuff ONCE regardless of hit count (e.g. one
 * attack's follow-up applies a DoT once at the end, not per hit), while other
 * effects (e.g. certain light cones) do attempt on every hit. This should be a
 * per-skill/per-effect configurable field, not auto-derived from hit count.
 */
struct EffectHitRateConfig {
    double baseChance;           // Base chance as decimal (e.g., 0.30 for 30%)
    double effectRES;            // Enemy's Effect RES as decimal (e.g., 0.20 for 20%)
    double ehr;                  // Effect Hit Rate as decimal (e.g., 0.50 for 50%)
    int hitCount;              // Manually specified per skill, NOT auto-derived
    
    EffectHitRateConfig()
        : baseChance(0.0)
        , effectRES(0.0)
        , ehr(0.0)
        , hitCount(1) {}
};

/**
 * Stack type: Punchline (Elation) or Banger (Certified Banger).
 * These are separate pools that never combine.
 */
enum class StackType {
    Punchline,   // Elation's Punchline stack multiplier
    Banger       // Certified Banger stack multiplier
};

/**
 * Configuration for Punchline/Banger stack damage multiplier.
 * 
 * DMG Multiplier = 1 + [(Stacks x 5) / (Stacks + 240)]
 * 
 * Rules:
 * - "Stacks" refers to either Elation's Punchline uses OR Certified Banger
 *   stacks — these are two SEPARATE pools that never combine/add together.
 * - No hard damage cap on this multiplier, but it has strong diminishing returns
 *   as stacks grow (natural property of the formula).
 * 
 * Input: stackCount (non-negative integer), stackType (Punchline or Banger)
 * Output: dmgMultiplier as decimal (e.g., 1.25 for 25% bonus)
 */
struct PunchlineBangerConfig {
    int stackCount;            // Number of stacks (0 or positive)
    StackType stackType;       // Punchline or Banger (mutually exclusive)
    
    PunchlineBangerConfig()
        : stackCount(0)
        , stackType(StackType::Punchline) {}
};

/**
 * Calculates the Resistance multiplier.
 * 
 * RES Mult = 100% - (RES% - RES PEN%)
 * 
 * Rules:
 * - Base enemy RES = 20% (Neutral), 0% (Weak), or 40% (Resistant)
 * - RES is clamped between -100% and 90% before applying PEN
 * - Final multiplier is clamped to range [0.1, 2.0]
 * 
 * @param config The resistance multiplier configuration
 * @return The calculated RES multiplier as a decimal (range 0.1 to 2.0)
 */
double calculateResistanceMultiplier(const ResistanceMultiplierConfig& config);

// Convenience overload with explicit base RES value
double calculateResistanceMultiplier(
    double enemyBaseRES,  // Explicit base RES (e.g., 0.20 for 20%)
    double resPenetration
);

// Convenience overload using resistance type
double calculateResistanceMultiplier(
    EnemyResistanceType resistanceType,
    double resPenetration
);

/**
 * Calculates the Vulnerability multiplier.
 * 
 * Vuln Multiplier = 1 + Sum VULN%, capped at x3.5 (250% VULN max).
 * The cap includes enemy self-vuln effects (e.g. Zandar's self-debuff)
 * that stack together with player-sourced VULN toward the same 250% cap.
 * 
 * There is also a separate, uncapped "Special Enemy Vuln" multiplier
 * applied only by Doomsday Beat and the story-only Sunday boss.
 * 
 * @param config The vulnerability configuration
 * @return The calculated vulnerability multiplier as a decimal
 */
double calculateVulnerabilityMultiplier(const VulnerabilityConfig& config);

// Convenience overload with sumVULN% and vuln type
double calculateVulnerabilityMultiplier(double sumVULN, EnemyVulnerabilityType vulnType = EnemyVulnerabilityType::Standard, double specialVulnEnemy = 0.0);

// Convenience overload with just sumVULN% (standard, capped)
double calculateVulnerabilityMultiplier(double sumVULN);

/**
 * Expected-value Crit multiplier (Section 21.4).
 *
 * critMult = 1 + clamp(critRate, 0, 1) x max(critDmg, 0)
 *
 * This is the average-damage convention: over many hits, expected damage
 * scales by this factor. It is applied at the engine layer (outside the
 * Section 1 master formula, which has no crit term) and exposed as an
 * intermediate for debugging/display.
 */
double calculateCritMultiplier(double critRate, double critDmg);
double calculateEffectHitRate(const EffectHitRateConfig& config);

// Convenience overload with individual parameters
double calculateEffectHitRate(double baseChance, double effectRES, double ehr, int hitCount = 1);

// Multi-hit at-least-one chance calculation
// At-Least-One Chance = 1 - (1 - Final Chance)^Hit Count
double calculateEffectHitRateAtLeastOne(double finalChancePerHit, int hitCount);

// Convenience overload using config
double calculateEffectHitRateAtLeastOne(const EffectHitRateConfig& config);

// Punchline/Banger stack damage multiplier
// DMG Multiplier = 1 + [(Stacks x 5) / (Stacks + 240)]
// Rules:
// - StackType::Punchline: Elation's Punchline stacks
// - StackType::Banger: Certified Banger stacks
// - These are SEPARATE pools, never combined
// - Strong diminishing returns as stacks grow (natural formula property)
double calculatePunchlineBangerMultiplier(const PunchlineBangerConfig& config);

// Convenience overload with individual parameters
double calculatePunchlineBangerMultiplier(int stackCount, StackType stackType);

// ============================================================================
// SECTION 6: DMG TAKEN MULTIPLIER CALCULATION
// ============================================================================
// DMG Taken Mult = 100% + Elemental DMG Taken% + All-Type DMG Taken%
//
// This comes from debuffs applied TO the enemy that increase damage they take
// (e.g. Welt's Ultimate, Sampo's Ultimate). Default to 0% if none active.

struct DamageTakenConfig {
    double elementalDMGTaken;   // Elemental DMG Taken% (only for matching element)
    double allTypeDMGTaken;     // All-Type DMG Taken%
    
    DamageTakenConfig()
        : elementalDMGTaken(0.0)
        , allTypeDMGTaken(0.0) {}
};

/**
 * Calculates the DMG Taken multiplier.
 * 
 * DMG Taken Mult = 100% + Elemental DMG Taken% + All-Type DMG Taken%
 * 
 * This includes debuffs applied to the enemy that increase damage they take
 * (e.g., Welt's Ultimate, Sampo's Ultimate).
 * 
 * @param config The DMG taken configuration
 * @return The calculated DMG Taken multiplier as a decimal
 */
double calculateDamageTakenMultiplier(const DamageTakenConfig& config);

// Convenience overload
double calculateDamageTakenMultiplier(
    double elementalDMGTaken = 0.0,
    double allTypeDMGTaken = 0.0
);

// ============================================================================
// SECTION 7: UNIVERSAL DMG REDUCTION MULTIPLIER
// ============================================================================
// Universal DMG Reduction Mult = 100% x (1 - Reduction_1) x (1 - Reduction_2) x ...
//
// Rules:
// - Multiple reduction sources stack MULTIPLICATIVELY, not additively.
// - Unbroken enemies with Toughness apply a built-in 10% reduction (0.90 multiplier).
// - Once the enemy is Broken, this built-in reduction becomes 0% (1.00 multiplier).

struct UniversalDamageReductionConfig {
    std::vector<double> reductionSources;  // Array of reduction decimals (e.g., [0.10, 0.15])
    bool isEnemyBroken;                    // If true, no built-in Toughness reduction
    
    UniversalDamageReductionConfig()
        : isEnemyBroken(false) {}
};

/**
 * Calculates the Universal DMG Reduction multiplier.
 * 
 * Universal DMG Reduction Mult = 100% x (1 - Reduction_1) x (1 - Reduction_2) x ...
 * 
 * Rules:
 * - Multiple reduction sources stack MULTIPLICATIVELY
 * - Unbroken enemies with Toughness apply a built-in 10% reduction (0.90 multiplier)
 * - Broken enemies have no built-in Toughness reduction (1.00 multiplier)
 * 
 * @param config The universal damage reduction configuration
 * @return The calculated universal reduction multiplier as a decimal
 */
double calculateUniversalDamageReductionMultiplier(const UniversalDamageReductionConfig& config);

// Convenience overload with single reduction value and broken state
double calculateUniversalDamageReductionMultiplier(
    double universalReduction,  // Single reduction source as decimal
    bool isEnemyBroken
);

// ============================================================================
// SECTION 8: WEAKENESS MULTIPLIER
// ============================================================================
// Weakeness Mult = 100% - Weakeness%
//
// Only relevant when calculating damage dealt BY enemies (e.g. Natasha's or
// Sampo's passive Weaken effect reducing incoming enemy damage). Default 0%
// (mult = 1.0) for player-character outgoing damage calculations.

struct WeakenessConfig {
    double weakenessPercent;  // Weakeness% as decimal (e.g., 0.20 for 20%), default 0.0
    
    WeakenessConfig()
        : weakenessPercent(0.0) {}
};

/**
 * Calculates the Weakeness multiplier.
 * 
 * Weakeness Mult = 100% - Weakeness%
 * 
 * Only relevant when calculating damage dealt BY enemies (e.g., Natasha's or
 * Sampo's passive Weaken effect reducing incoming enemy damage). Default 0%
 * (mult = 1.0) for player-character outgoing damage calculations.
 * 
 * @param config The weakeness configuration
 * @return The calculated weakeness multiplier as a decimal
 */
double calculateWeakenessMultiplier(const WeakenessConfig& config);

// Convenience overload
double calculateWeakenessMultiplier(double weakenessPercent = 0.0);

// ============================================================================
// SECTION 9: CORE STAT TOTALS (HP / ATK / DEF / SPEED)
// ============================================================================
// HP Total    = (Character Base HP  + LC Base HP)  x (1 + HP%)  + Flat HP
// ATK Total   = (Character Base ATK + LC Base ATK) x (1 + ATK%) + Flat ATK
// DEF Total   = (Character Base DEF + LC Base DEF) x (1 + DEF%) + Flat DEF
// Speed Total = Character Base Speed x (1 + Speed%) + Flat Speed
//
// Important rule: Light Cone BASE stats merge with character BASE stats FIRST,
// before percentage bonuses are applied. Do not apply % bonuses to LC base
// stats separately, or the result will be wrong.
//
// Inputs needed per stat: characterBase, lcBase (0 for Speed, LCs don't give base Speed),
//                         percentBonus (sum of all % substats/buffs), flatBonus (sum of all flat substats/buffs)
// Output: total value for HP, ATK, DEF, Speed

struct CoreStatConfig {
    double characterBase;    // Character's base stat value
    double lightConeBase;    // Light Cone's base stat (0 for Speed, as LCs don't give base Speed)
    double percentBonus;     // Sum of all % bonuses (substats, buffs, etc.) as decimal (e.g., 0.20 for 20%)
    double flatBonus;        // Sum of all flat bonuses (substats, buffs, etc.)
    
    CoreStatConfig()
        : characterBase(0.0)
        , lightConeBase(0.0)
        , percentBonus(0.0)
        , flatBonus(0.0) {}
};

/**
 * Calculates total HP.
 * 
 * HP Total = (Character Base HP + LC Base HP) x (1 + HP%) + Flat HP
 * 
 * @param config The HP configuration containing base values and bonuses
 * @return The calculated total HP
 */
double calculateTotalHP(const CoreStatConfig& config);

/**
 * Calculates total ATK.
 * 
 * ATK Total = (Character Base ATK + LC Base ATK) x (1 + ATK%) + Flat ATK
 * 
 * @param config The ATK configuration containing base values and bonuses
 * @return The calculated total ATK
 */
double calculateTotalATK(const CoreStatConfig& config);

/**
 * Calculates total DEF.
 * 
 * DEF Total = (Character Base DEF + LC Base DEF) x (1 + DEF%) + Flat DEF
 * 
 * @param config The DEF configuration containing base values and bonuses
 * @return The calculated total DEF
 */
double calculateTotalDEF(const CoreStatConfig& config);

/**
 * Calculates total Speed.
 * 
 * Speed Total = Character Base Speed x (1 + Speed%) + Flat Speed
 * Note: Light Cones do not provide base Speed, so lightConeBase should be 0.
 * 
 * @param config The Speed configuration containing base values and bonuses
 * @return The calculated total Speed
 */
double calculateTotalSpeed(const CoreStatConfig& config);

// Convenience overloads with individual parameters
double calculateTotalHP(double characterBase, double lightConeBase, double percentBonus, double flatBonus);
double calculateTotalATK(double characterBase, double lightConeBase, double percentBonus, double flatBonus);
double calculateTotalDEF(double characterBase, double lightConeBase, double percentBonus, double flatBonus);
// Convenience overloads with individual parameters
double calculateTotalHP(double characterBase, double lightConeBase, double percentBonus, double flatBonus);
double calculateTotalATK(double characterBase, double lightConeBase, double percentBonus, double flatBonus);
double calculateTotalDEF(double characterBase, double lightConeBase, double percentBonus, double flatBonus);
double calculateTotalSpeed(double characterBase, double percentBonus, double flatBonus);

/**
 * Calculates total speed with explicit Base Speed distinction.
 * 
 * Total Speed = Base Speed + (Base Speed × %Speed) + Flat Speed
 * 
 * Key rule: %Speed bonuses scale off BASE Speed only, NOT current/total Speed.
 * This means two characters with different current Speed totals but the same
 * Base Speed get an IDENTICAL Speed gain from the same %Speed buff.
 * 
 * Special case: Characters whose kit or Light Cone directly raises BASE Speed
 * (e.g. Aglaea's Light Cone) get proportionally more value from %Speed buffs
 * than characters who only add Speed via substats/boots (since substats/boots
 * add to the %Speed or Flat Speed terms, not Base Speed).
 * 
 * @param characterBase The character's base speed (before any % or flat bonuses)
 * @param percentBonus Speed% bonus as decimal (e.g., 0.10 for 10%)
 * @param flatBonus Flat Speed bonus (from boots, substats, etc.)
 * @return Total speed value
 */
double calculateSpeedWithBaseBonus(double characterBase, double percentBonus, double flatBonus);

/**
 * Calculates Action Advance - reduces the remaining AV requirement by a
 * percentage of the remaining requirement (not a flat amount).
 *
 * Rules:
 * - Advance is normally capped at 100% of the CURRENT remaining requirement
 *   (cannot push a turn to before 0 AV).
 * - EXCEPTION: if a character has previously been "delayed" such that they
 *   need MORE than 100% action to take their next turn (e.g. 120% required),
 *   the effective advance cap rises to match that higher requirement
 *   (e.g. up to 120%). This needs a delayedActionRequirement field that
 *   defaults to 100%.
 * - Action Advance and Speed have an inverse value relationship: faster
 *   characters have a smaller absolute AV pool remaining, so the same %
 *   Action Advance yields a smaller absolute time savings for them. This means
 *   Action Advance is worth MORE on slow characters and LESS on fast characters.
 *
 * Inputs: currentRemainingAVPercent (0.0 to delayedActionRequirement),
 *         advancePercent (0.0 to 1.0, as decimal or percentage),
 *         delayedActionRequirementPercent (defaults to 100.0, can be higher
 *           if character was previously delayed)
 * Output: newRemainingAVPercent (clamped such that it does not go below
 *         delayedActionRequirementPercent - meaning the result cannot be
 *         less than delayedActionRequirementPercent - currentRemainingAVPercent)
 */
double calculateActionAdvance(
    double currentRemainingAVPercent,
    double advancePercent,
    double delayedActionRequirementPercent = 100.0);

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
    WeakenessConfig weakenessConfig;
    VulnerabilityConfig vulnerabilityConfig;
    
    MasterDamageConfig()
        : dmgPercentMultiplier(1.0) {}
};

struct DamageResult {
    double baseDamage;
    double dmgPercentMultiplier;
    double defenseMultiplier;
    double resistanceMultiplier;
    double damageTakenMultiplier;
    double universalReductionMultiplier;
    double weakenessMultiplier;
    double vulnerabilityMultiplier;
    double finalDamage;
    
    DamageResult()
        : baseDamage(0.0)
        , dmgPercentMultiplier(1.0)
        , defenseMultiplier(1.0)
        , resistanceMultiplier(1.0)
        , damageTakenMultiplier(1.0)
        , universalReductionMultiplier(1.0)
        , weakenessMultiplier(1.0)
        , vulnerabilityMultiplier(1.0)
        , finalDamage(0.0) {}
};

/**
 * Calculates the full outgoing damage using the master formula.
 *
 * Outgoing DMG = Base DMG x DMG% Mult x DEF Mult x RES Mult x DMG Taken Mult
 *                x Universal DMG Reduction Mult x Weakeness Mult
 *
 * @param config The complete damage configuration
 * @return DamageResult containing all intermediate values and final damage
 */
DamageResult calculateOutgoingDamage(const MasterDamageConfig& config);

// ============================================================================
// SECTION 12/10: RELATIVE-GAIN COMPARISON TOOLS (declarations for the
// implementations already in DamageCalculator.cpp)
// ============================================================================
struct DefenseShredComparison {
    double currentMultiplier;
    double compareMultiplier;
    double relativeGainPercent;
};

DefenseShredComparison calculateDefenseShredComparison(
    double enemyTotalDEF,
    double shredPercent,
    double compareShredPercent,
    int attackerLevel,
    bool isPlightDifficulty);

struct ResistancePenComparison {
    double currentMultiplier;
    double compareMultiplier;
    double relativeGainPercent;
};

ResistancePenComparison calculateResistancePenetrationComparison(
    double enemyRES,
    double sumPEN,
    double comparePEN);

// SECTION 16: breakpoint disclaimer text for UI display.
const char* getSpeedBreakpointDisclaimer(void);

// SECTION 17: Action Advance (declaration; see header comment above impl).
// (calculateActionAdvance is already declared near calculateSpeedWithBaseBonus.)

// SECTION 18: Mid-turn speed change AV recalculation.
double calculateMidTurnSpeedChange(
    double oldSpeed,
    double avAlreadySpent,
    double actionAdvancePercentAlreadyApplied,
    double newSpeed);

} // namespace damage
} // namespace hsr

#endif // DAMAGE_CALCULATOR_H
