#include "SimulationEngine.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hsr {

SimulationEngine::SimulationEngine() : m_rng(42) {}

SimulationEngine::~SimulationEngine() {}

int SimulationEngine::calculateActionCost(int speed, const std::string& actionType) {
    int safeSpeed = std::max(1, speed);
    // Base AV = 10000 / speed (using integer math with 2 decimal precision)
    int baseAv = 10000 / safeSpeed;

    if (actionType == "Basic") {
        return baseAv;
    } else if (actionType == "Skill") {
        return baseAv; // Skill uses standard action
    } else if (actionType == "Heal" || actionType == "Shield") {
        return baseAv; // Support actions consume a full turn
    } else if (actionType == "Ult") {
        return 0; // Ultimate is instant, doesn't consume turn
    } else if (actionType == "FUA") {
        return 0; // Follow-up attack is instant
    } else if (actionType == "Memosprite") {
        return baseAv; // Memosprite summon action consumes a full turn
    }

    return baseAv;
}

double SimulationEngine::calculateTotalHp(const CharacterConfig& config) {
    // HP Total = (Character Base HP + LC Base HP) x (1 + HP%) + Flat HP
    double combinedBase = config.baseHp + config.lightConeBaseHp;
    return combinedBase * (1.0 + config.hpPct) + config.flatHp;
}

double SimulationEngine::calculateTotalAtk(const CharacterConfig& config) {
    // ATK Total = (Character Base ATK + LC Base ATK) x (1 + ATK%) + Flat ATK
    double combinedBase = config.baseAtk + config.lightConeBaseAtk;
    return combinedBase * (1.0 + config.atkPct) + config.flatAtk;
}

double SimulationEngine::calculateTotalDef(const CharacterConfig& config) {
    // DEF Total = (Character Base DEF + LC Base DEF) x (1 + DEF%) + Flat DEF
    double combinedBase = config.baseDef + config.lightConeBaseDef;
    return combinedBase * (1.0 + config.defPct) + config.flatDef;
}

double SimulationEngine::calculateTotalSpeed(const CharacterConfig& config) {
    // Speed Total = Character Base Speed x (1 + Speed%) + Flat Speed
    // Note: Light Cones do not provide base Speed, so lightConeBase should be 0
    return config.baseSpd * (1.0 + config.spdPct) + config.flatSpd;
}

int SimulationEngine::effectiveSpeed(const CharacterConfig& config) {
    // Section 21.8: manual entry and component builds are mutually exclusive
    // sources — never mixed for the same character.
    if (config.manualStats)
        return std::max(1, config.speed);
    return std::max(1, static_cast<int>(std::lround(calculateTotalSpeed(config))));
}

double SimulationEngine::effectiveStat(const CharacterConfig& config, const std::string& stat) {
    if (config.manualStats) {
        if (stat == "hp") return config.finalHp;
        if (stat == "def") return config.finalDef;
        return config.finalAtk;
    }
    if (stat == "hp") return calculateTotalHp(config);
    if (stat == "def") return calculateTotalDef(config);
    return calculateTotalAtk(config);
}

int SimulationEngine::calculateHitDamage(const CharState& charState,
                                         const EnemyState& enemyState,
                                         const std::string& actionType,
                                         ActionEvent& outEvent,
                                         double multOverride) {
    const CharacterConfig& c = charState.config;

    // Section 2: skill multiplier + scaling attribute from the single
    // source of truth (entered finals or component-built totals).
    // Phase 1: parsed per-skill multiplier wins when positive.
    double skillMult = c.basicMultiplier;
    if (actionType == "Skill") skillMult = c.skillMultiplier;
    else if (actionType == "Ult") skillMult = c.ultMultiplier;
    else if (actionType == "FUA") skillMult = c.fuaMultiplier;
    if (multOverride > 0.0)
        skillMult = multOverride;
    else {
        auto it = c.skillActions.find(actionType);
        if (it != c.skillActions.end() && it->second.damageMultiplier > 0.0)
            skillMult = it->second.damageMultiplier;
    }

    double scalingValue = effectiveStat(c, c.scalingStat);
    if (c.scalingStat == "atk")
        scalingValue *= (1.0 + c.buffAtkPct);

    damage::MasterDamageConfig dmg;
    dmg.baseDamageConfig.skillMultiplier = skillMult;
    dmg.baseDamageConfig.scalingAttributeValue = scalingValue;
    // Section 3 (+ starting buff DMG% + generic set "damage" bonuses).
    dmg.dmgPercentMultiplier = 1.0 + c.elementalDmgPct + c.allTypeDmgPct + c.buffDmgPct;
    // Section 4 + Section 12 shred.
    dmg.defenseConfig.attackerLevel = std::max(1, c.level);
    dmg.defenseConfig.enemyBaseDEF = enemyState.config.baseDef;
    dmg.defenseConfig.defIgnorePercent = c.defIgnorePct;
    dmg.defenseConfig.shredPercent = enemyState.config.defShredTaken;
    dmg.defenseConfig.isPlightDifficulty = enemyState.config.isPlightDifficulty;
    // Section 5 / Q2: explicit per-entry override wins; otherwise the
    // per-attacker-element auto-rule (when the attacker has an element);
    // otherwise the legacy type-bucket path.
    dmg.resistanceConfig.resistanceType = enemyState.config.resistanceType;
    dmg.resistanceConfig.explicitBaseRES = enemyState.config.baseResOverride;
    if (dmg.resistanceConfig.explicitBaseRES < 0.0 && !c.element.empty())
        dmg.resistanceConfig.explicitBaseRES = damage::resolveRES(
            enemyState.config.res, enemyState.config.weaknesses, c.element);
    dmg.resistanceConfig.resPenetration = c.resPen + enemyState.config.resReduction;
    // Section 6.
    dmg.damageTakenConfig.elementalDMGTaken = enemyState.config.dmgTakenElemental;
    dmg.damageTakenConfig.allTypeDMGTaken = enemyState.config.dmgTakenAll;
    // Section 7: broken state comes from live toughness tracking.
    dmg.universalReductionConfig.isEnemyBroken = enemyState.broken;
    // Section 8 defaults to 1.0 (player outgoing damage).
    // Section 11: standard pool (+ special pool for the 2 special enemies).
    dmg.vulnerabilityConfig.sumVULN = enemyState.config.vulnSum;
    if (enemyState.config.specialVuln > 0.0) {
        dmg.vulnerabilityConfig.vulnType = damage::EnemyVulnerabilityType::Special;
        dmg.vulnerabilityConfig.specialVulnEnemy = enemyState.config.specialVuln;
    }

    damage::DamageResult result = damage::calculateOutgoingDamage(dmg);
    // Section 21.4: expected-value crit, applied outside the Sec 1 formula.
    double critMult = damage::calculateCritMultiplier(c.critRate, c.critDmg);
    // Section 14: exactly one pool is ever active (stackPool selects it).
    double stackMult = 1.0;
    if (c.stackPool == 1)
        stackMult = damage::calculatePunchlineBangerMultiplier(c.stackCount, damage::StackType::Punchline);
    else if (c.stackPool == 2)
        stackMult = damage::calculatePunchlineBangerMultiplier(c.stackCount, damage::StackType::Banger);

    outEvent.damageBreakdown = result;
    outEvent.critMultiplier = critMult;
    outEvent.stackMultiplier = stackMult;

    double finalDamage = result.finalDamage * critMult * stackMult;
    return std::max(0, static_cast<int>(std::lround(finalDamage)));
}

size_t SimulationEngine::lowestHpAlly(const std::vector<CharState>& allies) {
    // Living allies only (hp > 0): no revive mechanics this phase, so the
    // dead are neither targeted nor healed back up.
    size_t best = 0;
    double bestFrac = 2.0;
    bool found = false;
    for (size_t i = 0; i < allies.size(); ++i) {
        if (allies[i].currentHp <= 0)
            continue;
        double maxHp = effectiveStat(allies[i].config, "hp");
        if (maxHp < 1.0) maxHp = 1.0;
        double frac = static_cast<double>(allies[i].currentHp) / maxHp;
        if (frac < bestFrac) {
            bestFrac = frac;
            best = i;
            found = true;
        }
    }
    if (!found && !allies.empty())
        return 0;
    return best;
}

int SimulationEngine::calculateEnemyHitDamage(const EnemyConfig& enemy,
                                             const CharacterConfig& ally) {
    // Documented minimal role mapping (pending an enemy-offense section):
    // enemy ATK x actionMultiplier, mitigated by the ally's total DEF via
    // the Sec 4 formula with attackerLevel = enemy level. Allies have no
    // toughness/RES/vuln/crit states, so those stages are 1.0.
    damage::DefenseMultiplierConfig def;
    def.attackerLevel = std::max(1, enemy.level);
    def.enemyBaseDEF = effectiveStat(ally, "def");
    double defMult = damage::calculateDefenseMultiplier(def);
    double raw = enemy.atk * std::max(0.0, enemy.actionMultiplier) * defMult;
    return std::max(0, static_cast<int>(std::lround(raw)));
}

bool SimulationEngine::applyEnemyAction(std::vector<CharState>& allies,
                                       EnemyState& enemy,
                                       int currentGlobalAv,
                                       std::vector<ActionEvent>& timeline) {
    ActionEvent event;
    event.characterId = enemy.config.id;
    event.characterName = enemy.config.name;
    event.targetEnemyId = enemy.config.id;
    event.currentAv = currentGlobalAv;
    event.damageDealt = 0;
    event.breakDamage = 0.0f;
    event.superBreakDamage = 0.0f;
    event.spChange = 0;
    event.critMultiplier = 1.0;
    event.stackMultiplier = 1.0;
    int enemySpd = 100;
    // Enemy AV cost display: threshold spacing (informational).
    enemySpd = static_cast<int>(std::lround(std::max(1.0, enemy.config.spd)));
    event.avCost = 10000 / std::max(1, enemySpd);
    event.isExtraTurn = false;

    // Phase 4.2: DoT ticks resolve at the start of the enemy's turn,
    // before recovery/attack. A tick-killed enemy neither recovers nor
    // attacks (its tick events are already on the timeline).
    if (!enemy.dots.empty()) {
        tickEnemyDots(enemy, currentGlobalAv, timeline);
        if (!enemy.active || enemy.currentHp <= 0)
            return false;
    }

    if (enemy.broken) {
        // Recovery turn (wiki): the turn is consumed restoring toughness
        // (current bar, full) and Exo-Toughness (full, re-depletable).
        // No attack is made on a recovery turn.
        event.actionType = "EnemyRecover";
        enemy.broken = false;
        if (enemy.barIndex < enemy.bars.size())
            enemy.currentToughness = enemy.bars[enemy.barIndex];
        if (enemy.config.exoToughness > 0) {
            enemy.currentExo = enemy.config.exoToughness;
            enemy.exoSpent = false;
        }
        timeline.push_back(event);
        return false;
    }

    event.actionType = "EnemyAtk";
    // Target: lowest-HP-fraction living ally (documented fallback).
    size_t targetIdx = lowestHpAlly(allies);
    if (allies[targetIdx].currentHp <= 0)
        return true; // No living ally (wipe detected by caller).
    event.targetAllyId = allies[targetIdx].config.id;

    int raw = calculateEnemyHitDamage(enemy.config, allies[targetIdx].config);
    event.damageDealt = raw;
    // Shields absorb first; overflow reaches HP.
    int absorbed = std::min(allies[targetIdx].shield, raw);
    allies[targetIdx].shield -= absorbed;
    event.shieldAbsorbed = static_cast<float>(absorbed);
    allies[targetIdx].currentHp = std::max(0, allies[targetIdx].currentHp - (raw - absorbed));

    // Phase 4.1: enemy splash. Blast = team-order-adjacent living allies,
    // AoE = all other living allies, each at full damage through their own
    // shield pool. Recorded in splashHits (enemyId holds the ally id).
    const std::string& ott = enemy.config.offenseTargetType;
    if (ott == "Blast" || ott == "AoE") {
        for (size_t i = 0; i < allies.size(); ++i) {
            if (i == targetIdx || allies[i].currentHp <= 0)
                continue;
            if (ott == "Blast") {
                size_t lo = (targetIdx > 0) ? targetIdx - 1 : 0;
                if (i < lo || i > targetIdx + 1)
                    continue;
            }
            int splashRaw = calculateEnemyHitDamage(enemy.config, allies[i].config);
            int splashAbsorbed = std::min(allies[i].shield, splashRaw);
            allies[i].shield -= splashAbsorbed;
            allies[i].currentHp = std::max(0, allies[i].currentHp - (splashRaw - splashAbsorbed));
            SplashHit hit;
            hit.enemyId = allies[i].config.id;
            hit.damageDealt = splashRaw;
            event.splashHits.push_back(hit);
        }
    }
    timeline.push_back(event);

    for (const auto& cs : allies) {
        if (cs.currentHp > 0)
            return false;
    }
    return true; // Party wiped.
}

void SimulationEngine::tryApplyBreakDot(const CharacterConfig& c,
                                           EnemyState& target,
                                           ActionEvent& ev) {
    if (c.breakDotType.empty() || c.breakDotChance <= 0.0 ||
        c.breakDotTurns <= 0 || c.breakDotAtkScale <= 0.0)
        return; // Unconfigured: no break DoT (default).
    // Sec 13: Final Chance = base x (1 - Effect RES) x (1 + EHR), capped.
    double chance = damage::calculateEffectHitRate(
        std::clamp(c.breakDotChance, 0.0, 1.0), target.config.effectRes,
        std::max(0.0, c.ehr), 1);
    double roll = std::uniform_real_distribution<double>(0.0, 1.0)(m_rng);
    if (roll >= chance)
        return; // Resisted.
    EnemyState::ActiveDot dot;
    dot.type = c.breakDotType;
    dot.remainingTicks = c.breakDotTurns;
    dot.atkScale = c.breakDotAtkScale;
    // Snapshot source stats (source may die; buffs may change mid-fight).
    dot.snapshotAtk = effectiveStat(c, c.scalingStat);
    if (c.scalingStat == "atk")
        dot.snapshotAtk *= (1.0 + c.buffAtkPct);
    dot.snapshotLevel = std::max(1, c.level);
    dot.snapshotElemDmg = c.elementalDmgPct;
    dot.snapshotAllDmg = c.allTypeDmgPct + c.buffDmgPct;
    dot.snapshotDotDmg = c.dotDmgPct;
    dot.snapshotResPen = c.resPen;
    dot.snapshotElement = c.element;
    target.dots.push_back(dot);
    ev.debuffApplied = c.breakDotType;
}

int SimulationEngine::tickEnemyDots(EnemyState& enemy,
                                    int currentGlobalAv,
                                    std::vector<ActionEvent>& timeline) {
    int total = 0;
    for (auto& dot : enemy.dots) {
        if (dot.remainingTicks <= 0)
            continue;
        // DoT tick: master-formula stages with includeDotDMG, no crit
        // (DoTs cannot crit), no stacks. Base = scale x snapshot ATK.
        damage::MasterDamageConfig dmg;
        dmg.baseDamageConfig.skillMultiplier = std::max(0.0, dot.atkScale);
        dmg.baseDamageConfig.scalingAttributeValue = std::max(0.0, dot.snapshotAtk);
        dmg.dmgPercentMultiplier = 1.0 + dot.snapshotElemDmg +
            dot.snapshotAllDmg + dot.snapshotDotDmg;
        dmg.defenseConfig.attackerLevel = dot.snapshotLevel;
        dmg.defenseConfig.enemyBaseDEF = enemy.config.baseDef;
        dmg.defenseConfig.shredPercent = enemy.config.defShredTaken;
        dmg.defenseConfig.isPlightDifficulty = enemy.config.isPlightDifficulty;
        dmg.resistanceConfig.resistanceType = enemy.config.resistanceType;
        dmg.resistanceConfig.explicitBaseRES = enemy.config.baseResOverride;
        if (dmg.resistanceConfig.explicitBaseRES < 0.0 && !dot.snapshotElement.empty())
            dmg.resistanceConfig.explicitBaseRES = damage::resolveRES(
                enemy.config.res, enemy.config.weaknesses, dot.snapshotElement);
        dmg.resistanceConfig.resPenetration =
            dot.snapshotResPen + enemy.config.resReduction;
        dmg.damageTakenConfig.elementalDMGTaken = enemy.config.dmgTakenElemental;
        dmg.damageTakenConfig.allTypeDMGTaken = enemy.config.dmgTakenAll;
        dmg.universalReductionConfig.isEnemyBroken = enemy.broken;
        dmg.vulnerabilityConfig.sumVULN = enemy.config.vulnSum;
        if (enemy.config.specialVuln > 0.0) {
            dmg.vulnerabilityConfig.vulnType = damage::EnemyVulnerabilityType::Special;
            dmg.vulnerabilityConfig.specialVulnEnemy = enemy.config.specialVuln;
        }
        damage::DamageResult result = damage::calculateOutgoingDamage(dmg);
        int amount = std::max(0, static_cast<int>(std::lround(result.finalDamage)));
        enemy.currentHp = std::max(0, enemy.currentHp - amount);
        --dot.remainingTicks;
        total += amount;

        ActionEvent tick;
        tick.characterId = "";
        tick.characterName = "DoT";
        tick.actionType = "DotTick";
        tick.targetEnemyId = enemy.config.id;
        tick.damageDealt = amount;
        tick.currentAv = currentGlobalAv;
        tick.avCost = 0;
        tick.isExtraTurn = false;
        tick.debuffApplied = dot.type;
        tick.damageBreakdown = result;
        tick.critMultiplier = 1.0;
        tick.stackMultiplier = 1.0;
        timeline.push_back(tick);
    }
    // Expire spent DoTs.
    enemy.dots.erase(
        std::remove_if(enemy.dots.begin(), enemy.dots.end(),
                       [](const EnemyState::ActiveDot& d) {
                           return d.remainingTicks <= 0;
                       }),
        enemy.dots.end());
    if (enemy.currentHp <= 0)
        enemy.active = false;
    return total;
}

float SimulationEngine::computeBreakEvent(const CharacterConfig& c, EnemyState& target,
                                         double barMaxToughness) {
    damage::BreakDamageConfig bc;
    bc.attackerLevel = std::max(1, c.level);
    bc.attackerElement = c.element;
    bc.breakEffect = c.breakEffect;
    bc.breakDmgIncrease = c.breakDmgIncrease;
    bc.enemyMaxToughness = barMaxToughness;
    bc.defenseConfig.attackerLevel = std::max(1, c.level);
    bc.defenseConfig.enemyBaseDEF = target.config.baseDef;
    bc.defenseConfig.defIgnorePercent = c.defIgnorePct;
    bc.defenseConfig.shredPercent = target.config.defShredTaken;
    bc.defenseConfig.isPlightDifficulty = target.config.isPlightDifficulty;
    bc.resistanceConfig.resistanceType = target.config.resistanceType;
    bc.resistanceConfig.explicitBaseRES = target.config.baseResOverride;
    if (bc.resistanceConfig.explicitBaseRES < 0.0 && !c.element.empty())
        bc.resistanceConfig.explicitBaseRES = damage::resolveRES(
            target.config.res, target.config.weaknesses, c.element);
    bc.resistanceConfig.resPenetration = c.resPen + target.config.resReduction;
    bc.vulnerabilityConfig.sumVULN = target.config.vulnSum;
    if (target.config.specialVuln > 0.0) {
        bc.vulnerabilityConfig.vulnType = damage::EnemyVulnerabilityType::Special;
        bc.vulnerabilityConfig.specialVulnEnemy = target.config.specialVuln;
    }
    // The break hit lands while the enemy still counts as unbroken
    // (x0.9, wiki) — compute BEFORE flipping the broken flag.
    bc.universalReductionConfig.isEnemyBroken = false;
    damage::BreakDamageResult br = damage::calculateBreakDamage(bc);
    int amount = static_cast<int>(std::lround(br.finalBreakDamage));
    target.currentHp = std::max(0, target.currentHp - amount);
    return static_cast<float>(amount);
}

void SimulationEngine::applyActionEffects(
    std::vector<CharState>& allies,
    size_t actorIdx,
    std::vector<EnemyState>& enemies,
    EnemyState* target,
    const std::string& actionType,
    int currentGlobalAv,
    std::vector<ActionEvent>& timeline) {

    (void)enemies;
    CharState& charState = allies[actorIdx];
    const bool isHeal = (actionType == "Heal");
    const bool isShield = (actionType == "Shield");
    const bool isSupport = isHeal || isShield;

    ActionEvent event;
    event.characterId = charState.config.id;
    event.characterName = charState.config.name;
    event.actionType = actionType;
    event.targetEnemyId = (target != nullptr && !isSupport) ? target->config.id : "";
    event.currentAv = currentGlobalAv;
    event.avCost = calculateActionCost(effectiveSpeed(charState.config), actionType);
    event.isExtraTurn = false;
    event.breakDamage = 0.0f;

    // Per-skill tuning (Sec 22 DB): present-and-positive wins, otherwise
    // the documented engine fallbacks below. Never guessed into the DB.
    const CharacterConfig::SkillActionTuning* tuning = nullptr;
    {
        auto it = charState.config.skillActions.find(actionType);
        if (it != charState.config.skillActions.end())
            tuning = &it->second;
    }

    if (isSupport) {
        // Heal/shield scale the character's scaling stat.
        // No enemy damage, no toughness interaction, no crit.
        double scalingValue = effectiveStat(charState.config, charState.config.scalingStat);
        size_t allyIdx = lowestHpAlly(allies);
        event.healTargetId = allies[allyIdx].config.id;
        if (isHeal) {
            double mult = (tuning != nullptr && tuning->healMultiplier > 0.0)
                ? tuning->healMultiplier : charState.config.healMultiplier;
            double amount = damage::calculateHealAmount(
                mult * scalingValue,
                charState.config.outgoingHealingBoost);
            int heal = std::max(0, static_cast<int>(std::lround(amount)));
            double maxHp = effectiveStat(allies[allyIdx].config, "hp");
            allies[allyIdx].currentHp = std::min(static_cast<int>(std::lround(maxHp)),
                                                allies[allyIdx].currentHp + heal);
            event.healAmount = static_cast<float>(heal);
        } else {
            // Shield formula: base x shieldBoost (wiki); shieldBoost sources
            // unmodeled. Shields stack and absorb enemy offense.
            double mult = (tuning != nullptr && tuning->shieldMultiplier > 0.0)
                ? tuning->shieldMultiplier : charState.config.shieldMultiplier;
            int shield = std::max(0, static_cast<int>(std::lround(
                mult * scalingValue)));
            allies[allyIdx].shield += shield;
            event.shieldAmount = static_cast<float>(shield);
        }
    } else {
        // Damage path is resolved per-target below (primary + splash), so
        // nothing is computed here. event.damageDealt is filled by the
        // primary hit in the target loop.
        event.damageDealt = 0;
    }

    // SP changes. Heal/Shield cost SP like Skill (fallback convention:
    // per-action SP costs are character data in the Sec 22 DB milestone).
    // Memosprite actions use their own summon resource: no SP change
    // (documented default; not guessed as a Skill-cost action).
    if (actionType == "Basic") {
        event.spChange = 1;
        charState.sp = std::min(charState.sp + 1, charState.config.maxSp);
    } else if (actionType == "Skill" || isSupport) {
        event.spChange = -1;
        charState.sp = std::max(charState.sp - 1, 0);
    } else {
        event.spChange = 0;
    }

    // Energy gain: parsed per-skill value wins when positive (scaled by
    // Energy Regen like the default); Ult already had the energy to fire.
    float energyGain;
    if (tuning != nullptr && tuning->energyGain > 0.0)
        energyGain = static_cast<float>(tuning->energyGain) *
            static_cast<float>(1.0 + std::max(0.0, charState.config.energyRegen));
    else
        energyGain = 20.0f * static_cast<float>(1.0 + std::max(0.0, charState.config.energyRegen));
    if (actionType == "Ult") energyGain = 0.0f;
    charState.energy = std::min(charState.energy + energyGain, charState.config.maxEnergy);

    if (!isSupport && target != nullptr) {
        // Primary-target toughness: parsed DB value wins when positive,
        // otherwise the documented fallback tiers.
        int primaryToughness = 20;
        if (tuning != nullptr && tuning->toughnessDamage > 0) {
            primaryToughness = tuning->toughnessDamage;
        } else if (actionType == "Basic") primaryToughness = 10;
        else if (actionType == "Skill") primaryToughness = 20;
        else if (actionType == "Ult") primaryToughness = 30;
        else if (actionType == "FUA") primaryToughness = 10;
        else if (actionType == "Memosprite") primaryToughness = 10;

        // Primary-target multiplier override (0 = configured multipliers).
        double primaryMult = (tuning != nullptr) ? tuning->damageMultiplier : 0.0;
        // Splash multiplier: parsed adjacent value wins, otherwise the
        // primary multiplier (documented fallback; never guessed).
        double splashMult = (tuning != nullptr && tuning->adjacentMultiplier > 0.0)
            ? tuning->adjacentMultiplier : primaryMult;

        // Primary hit fills event.damageDealt / event.breakDamage.
        applyHitToEnemy(charState, *target, actionType, tuning,
                        primaryMult, primaryToughness, event);

        // Splash resolution by parsed target type (data-driven, Sec 21).
        std::string tt = (tuning != nullptr) ? tuning->targetType : "";
        if (tt == "Blast" || tt == "AoE" || tt == "Bounce") {
            int splashToughness = primaryToughness;
            if (tt == "Blast" && tuning != nullptr)
                splashToughness = tuning->toughnessAdjacent;
            int bounceLeft = (tuning != nullptr) ? tuning->bounceHits : 0;
            for (auto& e : enemies) {
                if (&e == target || !e.active || e.currentHp <= 0)
                    continue;
                if (tt == "Blast") {
                    // Adjacent = slot-neighbors (slot +- 1), per the data's
                    // "to adjacent targets" wording and HSR Blast semantics.
                    int d = e.config.slotIndex - target->config.slotIndex;
                    if (d != 1 && d != -1)
                        continue;
                } else if (tt == "Bounce") {
                    // Deterministic slot-order bounce (random in game).
                    if (bounceLeft <= 0)
                        continue;
                    --bounceLeft;
                }
                // AoE: every other active, living enemy.
                ActionEvent splashEv;
                applyHitToEnemy(charState, e, actionType, tuning,
                                splashMult, splashToughness, splashEv);
                SplashHit hit;
                hit.enemyId = e.config.id;
                hit.damageDealt = splashEv.damageDealt;
                hit.breakDamage = splashEv.breakDamage;
                event.splashHits.push_back(hit);
                event.breakDamage += splashEv.breakDamage;
                if (e.currentHp <= 0)
                    e.active = false;
            }
        }
    }

    timeline.push_back(event);
}

// One hit (primary or splash) against a single enemy.
void SimulationEngine::applyHitToEnemy(
    CharState& charState,
    EnemyState& target,
    const std::string& actionType,
    const CharacterConfig::SkillActionTuning* tuning,
    double multOverride,
    int toughnessDamage,
    ActionEvent& ev) {
    (void)tuning;
    ev.damageDealt = 0;
    ev.breakDamage = 0.0f;
    ev.superBreakDamage = 0.0f;

    // Section 1 master formula via the implemented Sections 2-8/11 stages.
    ev.damageDealt = calculateHitDamage(charState, target, actionType, ev,
                                        multOverride);

    // Apply damage to the target enemy
    target.currentHp = std::max(0, target.currentHp - ev.damageDealt);

    const CharacterConfig& c = charState.config;
    if (ev.damageDealt > 0 && !target.broken) {
        target.currentToughness = std::max(0, target.currentToughness - toughnessDamage);
        if (target.currentToughness == 0) {
            if (target.barIndex + 1 < target.bars.size()) {
                // Non-final layer (wiki multi-layered toughness): Break
                // DMG only — no delay, no debuff, no broken state.
                ev.breakDamage = computeBreakEvent(
                    c, target,
                    static_cast<double>(target.bars[target.barIndex]));
                target.barIndex++;
                target.currentToughness = target.bars[target.barIndex];
            } else {
                // Final bar: full Weakness Break event.
                ev.breakDamage = computeBreakEvent(
                    c, target,
                    static_cast<double>(target.bars[target.barIndex]));
                target.broken = true;
                // Phase 4.2: EHR-gated break DoT (no-op unless configured).
                tryApplyBreakDot(c, target, ev);
            }
        }
    } else if (ev.damageDealt > 0 && target.broken) {
            // Super Break (documented simplification): attacker converts
            // toughness damage dealt to the broken enemy while Exo also
            // depletes independently below. Trigger rules: modifier gate,
            // plus per-action allowlist when configured (empty = all
            // damaging types); efficiency scales the converted amount.
            // Hard exceptions (protection-state breakers) are not modeled.
            bool superAllowed = (c.superBreakModifier > 0.0);
            if (superAllowed && !c.superBreakActions.empty()) {
                superAllowed = false;
                for (const auto& allowed : c.superBreakActions) {
                    if (allowed == actionType) {
                        superAllowed = true;
                        break;
                    }
                }
            }
            if (superAllowed) {
                damage::UniversalDamageReductionConfig uniBroken;
                uniBroken.isEnemyBroken = true;
                double effectiveTough = static_cast<double>(toughnessDamage) *
                    (1.0 + std::max(0.0, c.breakEfficiencyBoost));
                double superBreak = damage::calculateSuperBreakDamage(
                    effectiveTough,
                    c.breakEffect, c.superBreakModifier,
                    ev.damageBreakdown.defenseMultiplier,
                    ev.damageBreakdown.resistanceMultiplier,
                    ev.damageBreakdown.vulnerabilityMultiplier,
                    damage::calculateUniversalDamageReductionMultiplier(uniBroken));
                int superAmount = std::max(0, static_cast<int>(std::lround(superBreak)));
                ev.superBreakDamage = static_cast<float>(superAmount);
                ev.breakDamage += ev.superBreakDamage;
                target.currentHp = std::max(0, target.currentHp - superAmount);
            }
            // Exo-Toughness (wiki): any further hits deplete it; at zero it
            // triggers a second full break event, then stays spent.
            if (target.currentExo > 0) {
                target.currentExo = std::max(0, target.currentExo - toughnessDamage);
                if (target.currentExo == 0 && !target.exoSpent) {
                    target.exoSpent = true;
                    ev.breakDamage += computeBreakEvent(
                        c, target,
                        static_cast<double>(target.config.exoToughness));
                }
            }
        }
        if (target.currentHp <= 0)
            target.active = false;
}

bool SimulationEngine::allEnemiesDefeated(const std::vector<EnemyState>& enemies, int avLimit) {
    for (const auto& e : enemies) {
        if (e.active && e.currentHp > 0)
            return false;
        // A pending spawn inside the AV limit means combat is not over.
        if (!e.active && e.currentHp > 0 && e.config.spawnAv > 0 && e.config.spawnAv <= avLimit)
            return false;
    }
    return true;
}

bool SimulationEngine::checkZeroCycleClear(const std::vector<EnemyState>& enemies, int currentAv) {
    // Single-wave convenience predicate: everything currently known is
    // resolved and the kill happened at or under 150.00 AV. NOTE: the main
    // loop does NOT use this — it checks allEnemiesDefeated(enemies,
    // avLimit) so pending in-limit spawns correctly block a clear.
    return allEnemiesDefeated(enemies, currentAv) && currentAv <= 15000;
}

std::vector<SimulationEngine::SpeedBreakpoint> SimulationEngine::calculateBreakpoints(
    int baseSpeed, int avLimit) {

    std::vector<SimulationEngine::SpeedBreakpoint> breakpoints;

    // Calculate how many actions needed for different targets
    for (int targetActions = 1; targetActions <= 5; ++targetActions) {
        SpeedBreakpoint bp;
        bp.targetActions = targetActions;

        // Speed needed = (10000 * targetActions) / avLimit
        // For 150 AV limit: Speed = (10000 * actions) / 15000
        bp.requiredSpeed = (10000 * targetActions + avLimit - 1) / avLimit; // Ceiling division

        bp.description = std::to_string(targetActions) + " actions in 150 AV";

        breakpoints.push_back(bp);
    }

    return breakpoints;
}

SimulationResult SimulationEngine::runSimulation(
    const std::vector<CharacterConfig>& characters,
    const EncounterConfig& encounter,
    int avLimit) {

    SimulationResult result;
    result.success = false;
    result.totalCycles = 0;
    result.totalActions = 0;
    result.totalDamage = 0.0f;
    result.totalBreakDamage = 0.0f;
    result.isZeroCycleClear = false;

    if (characters.empty()) {
        result.errorMessage = "No characters provided";
        return result;
    }

    // Initialize persistent combat state (Sec 21.6).
    std::vector<CharState> charStates;
    for (const auto& config : characters) {
        CharState state;
        state.config = config;
        state.actionIndex = 0;
        state.sp = config.currentSp;
        state.energy = config.energy;
        // Live HP pool for heal targeting (clamped: a 0-HP actor is nonsense).
        state.currentHp = std::max(1, static_cast<int>(std::lround(effectiveStat(config, "hp"))));
        state.turnStartSpeed = effectiveSpeed(config);
        state.scheduleBaseAv = 0;
        // First-turn threshold from effective speed (Sec 15/16).
        state.currentAv = 10000 / state.turnStartSpeed;
        charStates.push_back(state);
    }

    auto enemyAvCost = [](double spd) {
        return 10000 / std::max(1, static_cast<int>(std::lround(std::max(1.0, spd))));
    };

    std::vector<EnemyState> enemyStates;
    for (const auto& e : encounter.flatten()) {
        EnemyState state;
        state.config = e;
        state.currentHp = e.currentHp;
        // Multi-layered bars, or the legacy single bar (wiki).
        state.bars = e.toughnessBars.empty()
            ? std::vector<int>{e.toughness}
            : e.toughnessBars;
        state.barIndex = 0;
        state.currentToughness = state.bars[0];
        state.currentExo = std::max(0, e.exoToughness);
        state.active = (e.spawnAv <= 0);
        state.broken = false;
        // First turn threshold from enemy SPD (present) or spawn clock.
        state.currentAv = (e.spawnAv <= 0) ? enemyAvCost(e.spd)
                                           : e.spawnAv + enemyAvCost(e.spd);
        enemyStates.push_back(state);
    }

    if (enemyStates.empty()) {
        result.errorMessage = "No enemies in encounter";
        return result;
    }

    int currentGlobalAv = 0;

    // Main simulation loop - process actions in AV order.
    while (currentGlobalAv <= avLimit) {
        // Activate scheduled spawns (Sec 21.3: dynamic mid-combat entry).
        for (auto& e : enemyStates) {
            if (!e.active && e.currentHp > 0 && e.config.spawnAv > 0 &&
                e.config.spawnAv <= currentGlobalAv) {
                e.active = true;
                ActionEvent spawnEvent;
                spawnEvent.actionType = "Spawn";
                spawnEvent.characterName = e.config.name;
                spawnEvent.targetEnemyId = e.config.id;
                spawnEvent.currentAv = currentGlobalAv;
                result.timeline.push_back(spawnEvent);
            }
        }

        if (allEnemiesDefeated(enemyStates, avLimit))
            break;

        // Find next pending spawn to fast-forward to when nothing is active.
        bool anyTarget = false;
        for (const auto& e : enemyStates) {
            if (e.active && e.currentHp > 0) {
                anyTarget = true;
                break;
            }
        }
        if (!anyTarget) {
            int nextSpawn = INT32_MAX;
            for (const auto& e : enemyStates) {
                if (!e.active && e.currentHp > 0 && e.config.spawnAv > currentGlobalAv)
                    nextSpawn = std::min(nextSpawn, e.config.spawnAv);
            }
            if (nextSpawn == INT32_MAX || nextSpawn > avLimit)
                break;
            currentGlobalAv = nextSpawn;
            for (auto& cs : charStates)
                cs.currentAv = std::max(cs.currentAv, nextSpawn);
            continue;
        }

        // Next actor: lowest AV among living allies and active enemies.
        // Dead allies (hp <= 0) never act; dead/inactive enemies neither.
        int actingCharIdx = -1;
        int actingEnemyIdx = -1;
        int minAv = INT32_MAX;

        for (size_t i = 0; i < charStates.size(); ++i) {
            if (charStates[i].currentHp > 0 && charStates[i].currentAv < minAv) {
                minAv = charStates[i].currentAv;
                actingCharIdx = static_cast<int>(i);
            }
        }
        for (size_t i = 0; i < enemyStates.size(); ++i) {
            if (enemyStates[i].active && enemyStates[i].currentHp > 0 &&
                enemyStates[i].currentAv < minAv) {
                minAv = enemyStates[i].currentAv;
                actingCharIdx = -1;
                actingEnemyIdx = static_cast<int>(i);
            }
        }

        if (actingCharIdx == -1 && actingEnemyIdx == -1) break;

        if (actingEnemyIdx >= 0) {
            // ---- Enemy turn (offense / recovery) ----
            EnemyState& actingEnemy = enemyStates[static_cast<size_t>(actingEnemyIdx)];
            currentGlobalAv = actingEnemy.currentAv;
            if (currentGlobalAv > avLimit) break;
            size_t timelineBefore = result.timeline.size();
            bool wiped = applyEnemyAction(charStates, actingEnemy, currentGlobalAv, result.timeline);
            // Phase 4.2: DoT tick damage counts toward totals.
            for (size_t i = timelineBefore; i < result.timeline.size(); ++i) {
                if (result.timeline[i].actionType == "DotTick")
                    result.totalDamage += result.timeline[i].damageDealt;
            }
            actingEnemy.currentAv += enemyAvCost(actingEnemy.config.spd);
            if (wiped) {
                result.partyWiped = true;
                result.success = false;
                result.errorMessage = "Party wiped by enemy offense";
                result.totalCycles = (currentGlobalAv + 14999) / 15000;
                break;
            }
            continue;
        }

        CharState& actingChar = charStates[static_cast<size_t>(actingCharIdx)];
        currentGlobalAv = actingChar.currentAv;
        if (currentGlobalAv > avLimit) break;

        // Section 18: rescale the pending threshold if effective speed
        // changed since this turn was scheduled (no-op for static buffs).
        int effSpeed = effectiveSpeed(actingChar.config);
        if (effSpeed != actingChar.turnStartSpeed && actingChar.turnStartSpeed > 0) {
            double spent = static_cast<double>(currentGlobalAv - actingChar.scheduleBaseAv);
            double total = damage::calculateMidTurnSpeedChange(
                static_cast<double>(actingChar.turnStartSpeed), spent, 0.0,
                static_cast<double>(effSpeed));
            double remaining = total - spent;
            if (remaining < 0.0) remaining = 0.0;
            actingChar.currentAv = currentGlobalAv + static_cast<int>(std::lround(remaining));
            actingChar.turnStartSpeed = effSpeed;
            currentGlobalAv = actingChar.currentAv;
            if (currentGlobalAv > avLimit) break;
        }

        // Target: first active, living enemy in slot order (Sec 22.9).
        EnemyState* target = nullptr;
        for (auto& e : enemyStates) {
            if (e.active && e.currentHp > 0) {
                target = &e;
                break;
            }
        }
        if (target == nullptr) continue;

        // Determine action to take
        std::string actionToTake;

        if (actingChar.config.isAuto) {
            // Simple AI: Use skill if SP >= 1 and we have it in rotation, else basic
            bool hasSkillInRotation = false;
            for (const auto& act : actingChar.config.rotation) {
                if (act == "Skill") {
                    hasSkillInRotation = true;
                    break;
                }
            }

            if (hasSkillInRotation && actingChar.sp >= 1 && actingChar.energy >= actingChar.config.maxEnergy * 0.5f) {
                actionToTake = "Skill";
            } else {
                actionToTake = "Basic";
            }
        } else {
            // Use configured rotation
            if (actingChar.actionIndex < static_cast<int>(actingChar.config.rotation.size())) {
                actionToTake = actingChar.config.rotation[actingChar.actionIndex];
                actingChar.actionIndex++;
            } else {
                // Loop rotation or default to basic
                if (!actingChar.config.rotation.empty()) {
                    actingChar.actionIndex = 0;
                    actionToTake = actingChar.config.rotation[0];
                } else {
                    actionToTake = "Basic";
                }
            }
        }

        // Check if we can actually perform the action (SP check for
        // SP-costing actions; the auto-AI only picks Basic/Skill).
        if ((actionToTake == "Skill" || actionToTake == "Heal" ||
             actionToTake == "Shield") && actingChar.sp < 1) {
            actionToTake = "Basic"; // Fallback to basic if no SP
        }

        // Apply action effects
        size_t actorIdx = static_cast<size_t>(actingCharIdx);
        applyActionEffects(charStates, actorIdx, enemyStates, target, actionToTake, currentGlobalAv, result.timeline);

        // Update totals (damage actions only — Spawn events carry no damage).
        // Splash hits count toward total damage/break (multi-target, Phase 2).
        result.totalActions++;
        result.totalDamage += result.timeline.back().damageDealt;
        for (const auto& splash : result.timeline.back().splashHits)
            result.totalDamage += splash.damageDealt;
        result.totalBreakDamage += result.timeline.back().breakDamage;

        // Schedule the next turn: base cost, then Section 17 advance.
        int actionCost = calculateActionCost(effSpeed, actionToTake);
        actingChar.currentAv += actionCost;
        if (actionToTake == "Ult" && actingChar.config.ultAdvancePct > 0.0) {
            int nextCost = 10000 / effSpeed;
            double newRemainingPct = damage::calculateActionAdvance(
                100.0, actingChar.config.ultAdvancePct, actingChar.delayedRequirementPct);
            actingChar.currentAv += static_cast<int>(
                std::lround(nextCost * newRemainingPct / 100.0));
        }
        actingChar.turnStartSpeed = effSpeed;
        actingChar.scheduleBaseAv = currentGlobalAv;

        // Section 18 (all combatants): rescale anyone whose effective
        // speed changed as a result of this action.
        for (auto& cs : charStates) {
            int csEff = effectiveSpeed(cs.config);
            if (csEff != cs.turnStartSpeed && cs.turnStartSpeed > 0) {
                double spent = static_cast<double>(currentGlobalAv - cs.scheduleBaseAv);
                double total = damage::calculateMidTurnSpeedChange(
                    static_cast<double>(cs.turnStartSpeed), spent, 0.0,
                    static_cast<double>(csEff));
                double remaining = total - spent;
                if (remaining < 0.0) remaining = 0.0;
                cs.currentAv = currentGlobalAv + static_cast<int>(std::lround(remaining));
                cs.turnStartSpeed = csEff;
            }
        }

        // Check for zero cycle clear. Pending spawns inside the AV limit
        // must block the clear — hence avLimit, not currentGlobalAv.
        if (allEnemiesDefeated(enemyStates, avLimit) && currentGlobalAv <= 15000) {
            result.isZeroCycleClear = true;
            result.totalCycles = 0;
            break;
        }
    }

    // Determine final result (a wipe set its own outcome mid-loop).
    if (result.partyWiped) {
        // success=false + error already recorded; nothing to overwrite.
    } else if (allEnemiesDefeated(enemyStates, avLimit)) {
        result.success = true;
        if (result.isZeroCycleClear) {
            result.totalCycles = 0;
        } else {
            // Calculate cycles based on AV used
            result.totalCycles = (currentGlobalAv + 14999) / 15000; // Ceiling division
        }
    } else {
        result.success = false;
        result.errorMessage = "Enemy not defeated within AV limit";
        result.totalCycles = (currentGlobalAv + 14999) / 15000;
    }

    // Store final stats (single source of truth: effective values)
    for (const auto& state : charStates) {
        result.finalStats[state.config.id + "_sp"] = state.sp;
        result.finalStats[state.config.id + "_energy"] = static_cast<int>(state.energy);
        result.finalStats[state.config.id + "_hp"] =
            static_cast<int>(std::lround(effectiveStat(state.config, "hp")));
        result.finalStats[state.config.id + "_atk"] =
            static_cast<int>(std::lround(effectiveStat(state.config, "atk")));
        result.finalStats[state.config.id + "_def"] =
            static_cast<int>(std::lround(effectiveStat(state.config, "def")));
        result.finalStats[state.config.id + "_spd"] = effectiveSpeed(state.config);
        // Live pools (heal/shield state): current HP + absorb shield.
        result.finalStats[state.config.id + "_hpcur"] = state.currentHp;
        result.finalStats[state.config.id + "_shield"] = state.shield;
    }
    for (const auto& e : enemyStates) {
        result.finalStats["enemy_" + e.config.id + "_hp"] = e.currentHp;
    }

    return result;
}

SimulationResult SimulationEngine::runSimulation(
    const std::vector<CharacterConfig>& characters,
    const EnemyConfig& enemy,
    int avLimit) {

    EncounterConfig encounter;
    encounter.slots[0].push_back(enemy);
    return runSimulation(characters, encounter, avLimit);
}

} // namespace hsr
