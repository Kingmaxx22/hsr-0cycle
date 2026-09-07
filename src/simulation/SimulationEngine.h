#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace hsr {

// Represents a single action in the timeline
struct ActionEvent {
    int avCost;             // AV cost of this action (e.g., 10000 / SPD)
    int currentAv;          // Cumulative AV when this action happens
    std::string characterId;
    std::string characterName;
    std::string actionType; // "Basic", "Skill", "Ult", "FUA"
    int damageDealt;        // Simulated damage (placeholder for now)
    int spChange;           // SP generated/consumed
    float breakDamage;      // Break damage dealt
    bool isExtraTurn;       // True if caused by advance action
};

// Configuration for a single character in the simulation
struct CharacterConfig {
    std::string id;
    std::string name;
    int speed;
    int maxSp;
    int currentSp;
    float energy;
    float maxEnergy;
    std::vector<std::string> rotation; // e.g., {"Skill", "Basic", "Basic"}
    bool isAuto;            // If true, use simple AI logic
    std::string speedNotes;  // Character-specific speed/AV exception notes (informational only)

    // Base stats from character data (before LC/relic bonuses)
    double baseHp;
    double baseAtk;
    double baseDef;
    double baseSpd;

    // Percentage bonuses (from substats, relics, planar, etc.)
    double hpPct;
    double atkPct;
    double defPct;
    double spdPct;

    // Flat bonuses (from substats, relics, planar, etc.)
    double flatHp;
    double flatAtk;
    double flatDef;
    double flatSpd;

    // Light Cone base stats (merged with character BASE stats first, per formula rules)
    double lightConeBaseHp;
    double lightConeBaseAtk;
    double lightConeBaseDef;

    // Whether stats were manually entered (bypass component building workflow)
    bool manualStats;
};

// Configuration for the enemy
struct EnemyConfig {
    std::string id;
    std::string name;
    int maxHp;
    int currentHp;
    int toughness;
    float resistance;
};

// Result of a simulation run
struct SimulationResult {
    bool success;
    std::string errorMessage;
    int totalCycles;        // 0 if 0-cycle clear
    int totalActions;
    float totalDamage;
    float totalBreakDamage;
    std::vector<ActionEvent> timeline;
    std::map<std::string, int> finalStats; // Final SP, Energy per char
    bool isZeroCycleClear;  // True if enemy defeated within 150 AV
};

class SimulationEngine {
public:
    SimulationEngine();
    ~SimulationEngine();

    // Core Simulation
    SimulationResult runSimulation(
        const std::vector<CharacterConfig>& characters,
        const EnemyConfig& enemy,
        int avLimit = 15000 // 150.00 AV (using integer math for precision)
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

private:
    // Internal state for running simulation
    struct CharState {
        CharacterConfig config;
        int currentAv;      // Current AV threshold for next turn
        int actionIndex;    // Where in the rotation we are
        int sp;
        float energy;
    };

    struct EnemyState {
        EnemyConfig config;
        int currentHp;
        int currentToughness;
    };

    // Internal helpers
    int calculateActionCost(int speed, const std::string& actionType);
    void applyActionEffects(CharState& charState, EnemyState& enemyState, const std::string& actionType, std::vector<ActionEvent>& timeline);
    bool checkZeroCycleClear(const EnemyState& enemy, int currentAv);
};

} // namespace hsr

#endif // SIMULATION_ENGINE_H
