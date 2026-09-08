#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include "DamageCalculator.h"

#include <array>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace hsr {

// Represents a single action in the timeline
struct SplashHit {
    std::string enemyId;  // Adjacent/AoE/bounce target hit by splash
    int damageDealt = 0;  // Final damage on this target
    float breakDamage = 0.0f;
};
struct ActionEvent {
    int avCost;             // AV cost of this action (e.g., 10000 / SPD)
    int currentAv;          // Cumulative AV when this action happens
    std::string characterId;
    std::string characterName;
    std::string actionType; // "Basic", "Skill", "Ult", "FUA", "Memosprite", "Heal", "Shield", "Spawn"
    std::string targetEnemyId; // Primary enemy instance hit ("" for non-damage events)
    int damageDealt;        // Final damage on the PRIMARY target
    std::vector<SplashHit> splashHits; // Blast-adjacent / AoE / bounce hits
    int spChange;           // SP generated/consumed
    float breakDamage;      // Break (+ super break) damage dealt
    float superBreakDamage = 0.0f; // Super Break portion (also in breakDamage)
    float healAmount = 0.0f;  // HP restored (Heal actions)
    float shieldAmount = 0.0f;// Shield granted (Shield actions)
    std::string healTargetId; // Ally receiving heal/shield
    std::string targetAllyId; // Ally hit by enemy offense
    float shieldAbsorbed = 0.0f; // Incoming damage stopped by shield
    bool isExtraTurn;       // True if caused by advance action
    // Section 21.4: intermediate multipliers, exposed for debugging/display.
    damage::DamageResult damageBreakdown;
    double critMultiplier = 1.0;   // Expected-value crit factor (Sec 21.4)
    double stackMultiplier = 1.0;  // Punchline/Banger factor (Sec 14)
};

// Configuration for a single character in the simulation.
// Section 21.2 / 21.8: the engine receives calculated combat stats directly
// from the character/loadout system — it never asks for combined values
// twice. Each value below has a single source of truth upstream.
struct CharacterConfig {
    std::string id;
    std::string name;
    int speed = 100;        // Used ONLY when manualStats == true (entered SPD)
    int maxSp = 5;
    int currentSp = 3;
    float energy = 0.0f;
    float maxEnergy = 100.0f;
    std::vector<std::string> rotation; // e.g., {"Skill", "Basic", "Basic"}
    bool isAuto = false;    // If true, use simple AI logic
    int level = 80;         // Attacker level for the DEF formula (Sec 4)
    std::string element;    // Attacker element for per-element RES (Q2); "" = type-bucket fallback
    std::string speedNotes; // Character-specific speed/AV exception notes (informational only)

    // Base stats from character data (before LC/relic bonuses)
    double baseHp = 0.0;
    double baseAtk = 0.0;
    double baseDef = 0.0;
    double baseSpd = 0.0;

    // Percentage bonuses (from substats, relics, planar, etc.)
    double hpPct = 0.0;
    double atkPct = 0.0;
    double defPct = 0.0;
    double spdPct = 0.0;

    // Flat bonuses (from substats, relics, planar, etc.)
    double flatHp = 0.0;
    double flatAtk = 0.0;
    double flatDef = 0.0;
    double flatSpd = 0.0;

    // Light Cone base stats (merged with character BASE stats first, per formula rules)
    double lightConeBaseHp = 0.0;
    double lightConeBaseAtk = 0.0;
    double lightConeBaseDef = 0.0;

    // Whether stats were manually entered (bypass component building workflow)
    bool manualStats = false;
    // Entered final stats — read ONLY when manualStats == true (Sec 22.4).
    double finalHp = 0.0;
    double finalAtk = 0.0;
    double finalDef = 0.0;

    // --- Calculated combat stats (Sec 21.2), fed from the loadout system ---
    double critRate = 0.05;        // Base 5%
    double critDmg = 0.50;         // Base 50%
    double elementalDmgPct = 0.0;  // Elemental DMG% (Sec 3)
    double allTypeDmgPct = 0.0;    // All-Type DMG% (Sec 3)
    double resPen = 0.0;           // RES PEN, uncapped per Sec 11
    double defIgnorePct = 0.0;     // Ignore-DEF effects (Sec 4)
    double ehr = 0.0;              // Effect Hit Rate (Sec 13)
    double effectRes = 0.0;        // Effect RES (Sec 13/22.4; used by debuff application)
    double breakEffect = 0.0;        // 0..1 scale (e.g. 0.35 = 35%)
    double outgoingHealingBoost = 0.0; // decimals; see damage::calculateHealAmount
    double energyRegen = 0.0;        // decimals; scales energy gain
    // Break milestone: Break-DMG-Increase sources (e.g. Fugue E4).
    // No gear source (Eidolon-gated); configured per character.
    double breakDmgIncrease = 0.0;   // decimals; x(1+increase) on break hits
    // Super Break: 0 = off. >0 enables documented-simplification super
    // break instances on hits against already-broken enemies.
    double superBreakModifier = 0.0;
    // Heal/shield skill data (0 = this character has no such action).
    // Amounts scale the same scalingStat as damage.
    double healMultiplier = 0.0;
    double shieldMultiplier = 0.0;
    // Starting combat buffs (applied for the whole sim; per-turn
    // buff application rules are character-specific, Sec 29).
    double buffAtkPct = 0.0;
    double buffDmgPct = 0.0;

    // --- Damage skill data (per-action-type multipliers, ATK-scaling default) ---
    // Which total the skill scales off: "atk", "hp" or "def" (Sec 2).
    std::string scalingStat = "atk";
    double basicMultiplier = 1.0;
    double skillMultiplier = 1.0;
    double ultMultiplier = 1.0;
    double fuaMultiplier = 1.0;
    // % Action Advance applied when this character casts Ult (Sec 17).
    // 0 = none. Stored here (not invented per-character): data-driven later.
    double ultAdvancePct = 0.0;

    // Punchline/Banger stacks (Sec 14): mutually exclusive pools.
    // stackPool: 0 = none, 1 = punchline, 2 = banger.
    int stackCount = 0;
    int stackPool = 0;

    // Super Break trigger rules (beyond the modifier gate):
    // - superBreakActions: which of this character's action types convert
    //   toughness damage into super break instances. EMPTY = all damaging
    //   action types (back-compat default).
    // - breakEfficiencyBoost: multiplies the converted toughness damage
    //   (hsr-optimizer effectiveToughness form). 0 = no bonus.
    // Hard exceptions (e.g. toughness-protection breakers) are not modeled.
    std::vector<std::string> superBreakActions;
    double breakEfficiencyBoost = 0.0;

    // Per-skill tuning from the character DB (Sec 22 DB milestone).
    // Keys: "Basic", "Skill", "Ult", "FUA". Absent/zero entries fall back
    // to the documented engine defaults (toughness tiers, single
    // heal/shield multipliers). No values are guessed into the DB.
    struct SkillActionTuning {
        int toughnessDamage = 0;   // 0 = fallback tier
        int toughnessAdjacent = 0; // Adjacent-target toughness (Blast "+ N")
        double healMultiplier = 0.0;   // 0 = single healMultiplier field
        double shieldMultiplier = 0.0; // 0 = single shieldMultiplier field
        double energyGain = 0.0;       // 0 = engine default (20 x regen)
        double damageMultiplier = 0.0; // 0 = configured basic/skill/ult/fua
        double adjacentMultiplier = 0.0; // 0 = primary multiplier for splash
        int bounceHits = 0;            // Bounce extra instances (0 = none)
        // Targeting: "" (single fallback), "Single Target", "Blast",
        // "AoE", "Bounce".
        std::string targetType;
    };
    std::map<std::string, SkillActionTuning> skillActions;
};

// Configuration for a single enemy instance.
// Section 21.3: state needed for DEF/RES/vuln/shred interactions, slot
// assignment, and mid-combat spawning.
struct EnemyConfig {
    std::string id;
    std::string name;
    int maxHp = 1;
    int currentHp = 1;
    int toughness = 0;
    double atk = 0.0;               // Enemy ATK (offense damage base)
    double spd = 100.0;             // Enemy SPD (AV scheduling)
    // Enemy action damage multiplier (documented fallback 1.0; per-enemy
    // action data arrives with future enemy-data work).
    double actionMultiplier = 1.0;
    // Multi-layered toughness (wiki): non-empty = successive bars; only the
    // final bar triggers full break effects. Empty = legacy single bar.
    // No data source yet (monsters_rules.json has one toughness value).
    std::vector<int> toughnessBars;
    // Exo-Toughness (wiki): extra bar reduced after the main break, triggering
    // a second full break when depleted. 0 = none. Restored on recovery turn.
    int exoToughness = 0;
    int level = 80;                 // Enemy level (display/context; DEF uses baseDef)
    double baseDef = 0.0;           // Enemy total DEF before shred (Sec 4/12)
    // Base RES: explicit per-entry override wins when >= 0; otherwise the
    // Q2 auto-rule (attacker element) or the type bucket applies.
    double baseResOverride = -1.0;
    damage::EnemyResistanceType resistanceType = damage::EnemyResistanceType::Neutral;
    // Q2 enemy RES data (populated from the enemy DB at build time):
    // per-element override map (0..1) + weakness list. The engine resolves
    // per attacker element via damage::resolveRES (weakness-first).
    std::map<std::string, double> res;
    std::vector<std::string> weaknesses;
    // Debuffs currently on the enemy (Sec 6/11/12):
    double dmgTakenAll = 0.0;       // All-Type DMG Taken%
    double dmgTakenElemental = 0.0; // Elemental DMG Taken% (matching element)
    double vulnSum = 0.0;           // Standard VULN pool (cap 250% in formula)
    double specialVuln = 0.0;       // Special enemy vuln (2 enemies, uncapped)
    double defShredTaken = 0.0;     // DEF shred on this enemy (0..1, Sec 12)
    double resReduction = 0.0;      // Enemy RES reduction (adds to attacker PEN)
    bool isPlightDifficulty = false;// Lv120 w/ Lv100 DEF value (Sec 12)
    // Encounter placement (Sec 21.3 / 22.9): slot 0..4, activation AV for
    // enemies that spawn mid-combat (0 = present from the start).
    int slotIndex = 0;
    int spawnAv = 0;

    // Legacy field: approximate resistance kept for back-compat display.
    // The engine derives RES from resistanceType/baseResOverride instead.
    float resistance = 0.0f;
};

// Encounter: exactly five enemy slots (Sec 19/22.9); each slot holds
// zero or more enemy instances. Enemies with spawnAv > 0 enter combat
// dynamically without creating new slots.
struct EncounterConfig {
    std::array<std::vector<EnemyConfig>, 5> slots;

    // Flattened view (slot order, then insertion order) for the engine loop.
    std::vector<EnemyConfig> flatten() const {
        std::vector<EnemyConfig> out;
        for (size_t s = 0; s < slots.size(); ++s) {
            for (const auto& e : slots[s]) {
                EnemyConfig copy = e;
                copy.slotIndex = static_cast<int>(s);
                out.push_back(copy);
            }
        }
        return out;
    }
};

// Result of a simulation run
struct SimulationResult {
    bool success = false;
    std::string errorMessage;
    int totalCycles = 0;        // 0 if 0-cycle clear
    int totalActions = 0;
    float totalDamage = 0.0f;
    float totalBreakDamage = 0.0f;
    std::vector<ActionEvent> timeline;
    std::map<std::string, int> finalStats; // Final SP, Energy per char
    bool isZeroCycleClear = false;  // True if enemy defeated within 150 AV
    bool partyWiped = false;    // True if all allies fell (enemy offense)
};

class SimulationEngine {
public:
    SimulationEngine();
    ~SimulationEngine();

    // Core Simulation
    // Multi-enemy encounter entry point (Sec 21.3/21.6).
    SimulationResult runSimulation(
        const std::vector<CharacterConfig>& characters,
        const EncounterConfig& encounter,
        int avLimit = 15000 // 150.00 AV (using integer math for precision)
    );

    // Single-enemy convenience overload (wraps the enemy into slot 0).
    SimulationResult runSimulation(
        const std::vector<CharacterConfig>& characters,
        const EnemyConfig& enemy,
        int avLimit = 15000
    );

    // Helper to calculate speed breakpoints
    struct SpeedBreakpoint {
        int targetActions;
        int requiredSpeed;
        std::string description;
    };
    std::vector<SpeedBreakpoint> calculateBreakpoints(int baseSpeed, int avLimit = 15000);

public:
    // Stat calculation (derived from CharacterConfig components)
    // These preserve the formula stages: Base → % → Flat → LC merge
    // Public: usable by UI for both workflows display
    double calculateTotalHp(const CharacterConfig& config);
    double calculateTotalAtk(const CharacterConfig& config);
    double calculateTotalDef(const CharacterConfig& config);
    double calculateTotalSpeed(const CharacterConfig& config);

public:
    // Effective combat stats honoring the manualStats flag (Sec 21.8:
    // one source of truth — entered finals OR component-built totals).
    int effectiveSpeed(const CharacterConfig& config);
    double effectiveStat(const CharacterConfig& config, const std::string& stat);

private:
    // Internal state for running simulation (Sec 21.6: persistent combat
    // state across the whole run, not per-action recalculation).
    struct CharState {
        CharacterConfig config;
        int currentAv;      // Current AV threshold for next turn
        int actionIndex;    // Where in the rotation we are
        int sp;
        float energy;
        int currentHp;      // Live HP (heals restore up to max; no enemy
                            // offense exists yet, so nothing depletes it)
        int shield = 0;     // Absorb pool (consumer: future enemy offense)
        int turnStartSpeed; // Speed basis of the pending threshold (Sec 18)
        int scheduleBaseAv;   // AV clock when the pending threshold was set
        double delayedRequirementPct = 100.0; // Sec 17 advance-cap exception
    };

    struct EnemyState {
        EnemyConfig config;
        int currentAv = 0; // Next-turn AV threshold (10000/SPD scheduling)
        int currentHp;
        int currentToughness;
        size_t barIndex = 0; // Active multi-layer bar (bars vector copy below)
        std::vector<int> bars; // Resolved bars (config bars or {toughness})
        int currentExo = 0;  // Remaining Exo-Toughness (0 = none/spent)
        bool exoSpent = false; // Second-break event already fired
        bool active;        // False until spawnAv reached / after death
        bool broken;        // True once final toughness depleted (Sec 7)
    };

    // Internal helpers
    int calculateActionCost(int speed, const std::string& actionType);
    // Builds the Section 1 master-formula result for one hit (Sec 21.4),
    // filling intermediates for display. Returns final damage (rounded).
    int calculateHitDamage(const CharState& charState, const EnemyState& enemyState,
                           const std::string& actionType, ActionEvent& outEvent,
                           double multOverride = 0.0);
    void applyActionEffects(std::vector<CharState>& allies,
                            size_t actorIdx,
                            std::vector<EnemyState>& enemies,
                            EnemyState* target,
                            const std::string& actionType,
                            int currentGlobalAv,
                            std::vector<ActionEvent>& timeline);
    // One hit (primary or splash) against a single enemy: master-formula
    // damage into ev.damageDealt, HP/toughness/break/super-break/exo state
    // updates, break portion into ev.breakDamage. Caller aggregates.
    void applyHitToEnemy(CharState& charState,
                         EnemyState& target,
                         const std::string& actionType,
                         const CharacterConfig::SkillActionTuning* tuning,
                         double multOverride,
                         int toughnessOverride,
                         ActionEvent& ev);
    // Enemy turn: recovery when broken, otherwise an attack on the lowest-
    // HP-fraction living ally. Shields absorb first; overflow hits HP.
    // Returns true if the party was wiped by this action.
    bool applyEnemyAction(std::vector<CharState>& allies,
                          EnemyState& enemy,
                          int currentGlobalAv,
                          std::vector<ActionEvent>& timeline);
    // Enemy offense damage (documented minimal role mapping, pending an
    // enemy-offense formula section): ATK x actionMultiplier x ally DEF
    // multiplier (Sec 4, attackerLevel = enemy level). No toughness, RES,
    // vuln, or crit on either side.
    int calculateEnemyHitDamage(const EnemyConfig& enemy,
                                const CharacterConfig& ally);
    // Lowest-HP-fraction ally (heal/shield fallback targeting, documented).
    size_t lowestHpAlly(const std::vector<CharState>& allies);
    // Shared break-event computation (layers, final, exo share it).
    float computeBreakEvent(const CharacterConfig& c, EnemyState& target,
                            double barMaxToughness);
    bool allEnemiesDefeated(const std::vector<EnemyState>& enemies, int avLimit);
    bool checkZeroCycleClear(const std::vector<EnemyState>& enemies, int currentAv);
};

} // namespace hsr

#endif // SIMULATION_ENGINE_H
