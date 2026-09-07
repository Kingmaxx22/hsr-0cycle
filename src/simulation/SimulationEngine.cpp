#include "SimulationEngine.h"
#include <algorithm>
#include <cmath>

namespace hsr {

SimulationEngine::SimulationEngine() {}

SimulationEngine::~SimulationEngine() {}

int SimulationEngine::calculateActionCost(int speed, const std::string& actionType) {
    // Base AV = 10000 / speed (using integer math with 2 decimal precision)
    int baseAv = 10000 / speed;
    
    // Action modifiers (these are simplified - real HSR has more complex formulas)
    if (actionType == "Basic") {
        return baseAv;
    } else if (actionType == "Skill") {
        return baseAv; // Skill uses standard action
    } else if (actionType == "Ult") {
        return 0; // Ultimate is instant, doesn't consume turn
    } else if (actionType == "FUA") {
        return 0; // Follow-up attack is instant
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

void SimulationEngine::applyActionEffects(
    CharState& charState, 
    EnemyState& enemyState, 
    const std::string& actionType,
    std::vector<ActionEvent>& timeline) {
    
    ActionEvent event;
    event.characterId = charState.config.id;
    event.characterName = charState.config.name;
    event.actionType = actionType;
    event.avCost = calculateActionCost(charState.config.speed, actionType);
    event.isExtraTurn = false;
    
    // Simplified damage calculation (placeholder - would need full damage formula)
    int baseDamage = 1000;
    if (actionType == "Skill") baseDamage = 2500;
    else if (actionType == "Ult") baseDamage = 4000;
    else if (actionType == "FUA") baseDamage = 1500;
    
    event.damageDealt = static_cast<int>(baseDamage * (1.0f - enemyState.config.resistance));
    event.breakDamage = 0.0f;
    
    // SP changes
    if (actionType == "Basic") {
        event.spChange = 1;
        charState.sp = std::min(charState.sp + 1, charState.config.maxSp);
    } else if (actionType == "Skill") {
        event.spChange = -1;
        charState.sp = std::max(charState.sp - 1, 0);
    } else {
        event.spChange = 0;
    }
    
    // Energy gain (simplified)
    float energyGain = 20.0f;
    if (actionType == "Ult") energyGain = 0; // Already have energy to ult
    charState.energy = std::min(charState.energy + energyGain, charState.config.maxEnergy);
    
    // Apply damage to enemy
    enemyState.currentHp = std::max(0, enemyState.currentHp - event.damageDealt);
    
    // Check for toughness break (simplified)
    if (event.damageDealt > 0) {
        enemyState.currentToughness = std::max(0, enemyState.currentToughness - 30);
        if (enemyState.currentToughness == 0) {
            event.breakDamage = 1000.0f; // Break damage bonus
            enemyState.currentToughness = enemyState.config.toughness; // Reset after break
        }
    }
    
    timeline.push_back(event);
}

bool SimulationEngine::checkZeroCycleClear(const EnemyState& enemy, int currentAv) {
    return enemy.currentHp <= 0 && currentAv <= 15000;
}

std::vector<SimulationEngine::SpeedBreakpoint> SimulationEngine::calculateBreakpoints(
    int baseSpeed, int avLimit) {
    
    std::vector<SpeedBreakpoint> breakpoints;
    
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
    const EnemyConfig& enemy,
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
    
    // Initialize states
    std::vector<CharState> charStates;
    for (const auto& config : characters) {
        CharState state;
        state.config = config;
        state.currentAv = 0;
        state.actionIndex = 0;
        state.sp = config.currentSp;
        state.energy = config.energy;
        charStates.push_back(state);
    }
    
    EnemyState enemyState;
    enemyState.config = enemy;
    enemyState.currentHp = enemy.currentHp;
    enemyState.currentToughness = enemy.toughness;
    
    int currentGlobalAv = 0;
    
    // Main simulation loop - process actions in AV order
    while (currentGlobalAv <= avLimit && enemyState.currentHp > 0) {
        // Find character with lowest currentAv (next to act)
        int actingCharIdx = -1;
        int minAv = INT32_MAX;
        
        for (size_t i = 0; i < charStates.size(); ++i) {
            if (charStates[i].currentAv < minAv) {
                minAv = charStates[i].currentAv;
                actingCharIdx = static_cast<int>(i);
            }
        }
        
        if (actingCharIdx == -1) break;
        
        CharState& actingChar = charStates[actingCharIdx];
        currentGlobalAv = actingChar.currentAv;
        
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
        
        // Check if we can actually perform the action (SP check for skill)
        if (actionToTake == "Skill" && actingChar.sp < 1) {
            actionToTake = "Basic"; // Fallback to basic if no SP
        }
        
        // Apply action effects
        applyActionEffects(actingChar, enemyState, actionToTake, result.timeline);
        
        // Update totals
        result.totalActions++;
        result.totalDamage += result.timeline.back().damageDealt;
        result.totalBreakDamage += result.timeline.back().breakDamage;
        
        // Advance character's AV for next turn
        int actionCost = calculateActionCost(actingChar.config.speed, actionToTake);
        actingChar.currentAv += actionCost;
        
        // Check for zero cycle clear
        if (checkZeroCycleClear(enemyState, currentGlobalAv)) {
            result.isZeroCycleClear = true;
            result.totalCycles = 0;
            break;
        }
    }
    
    // Determine final result
    if (enemyState.currentHp <= 0) {
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
    
    // Store final stats
    for (const auto& state : charStates) {
        result.finalStats[state.config.id + "_sp"] = state.sp;
        result.finalStats[state.config.id + "_energy"] = static_cast<int>(state.energy);
        
        // Store calculated combat stats (from component build workflow)
        result.finalStats[state.config.id + "_hp"] = static_cast<int>(
            calculateTotalHp(state.config));
        result.finalStats[state.config.id + "_atk"] = static_cast<int>(
            calculateTotalAtk(state.config));
        result.finalStats[state.config.id + "_def"] = static_cast<int>(
            calculateTotalDef(state.config));
        result.finalStats[state.config.id + "_spd"] = static_cast<int>(
            calculateTotalSpeed(state.config));
    }
    
    return result;
}

} // namespace hsr
