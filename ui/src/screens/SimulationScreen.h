#pragma once

#include "Screen.h"
#include "../assets/AssetManager.h"
#include "../data/EnemyDatabase.h"
#include "../../src/simulation/SimulationEngine.h"
#include "../../src/simulation/EngineBridge.h"
#include "raylib.h"

#include <string>
#include <vector>
#include <map>

class SimulationScreen : public Screen
{
public:
    SimulationScreen(AssetManager& assets, EnemyDatabase& enemies);
    ~SimulationScreen();

    void initialize() override;
    void update(float dt) override;
    void draw() override;

    // Set the enemy to simulate against
    void setSelectedEnemy(const std::string& enemyId);
    
    // Add a character to the simulation
    void addCharacter(const hsr::CharacterConfig& config);
    
    // Remove a character from simulation
    void removeCharacter(size_t index);
    
    // Clear all characters
    void clearCharacters();
    
    // Run simulation
    void runSimulation();
    
    // Get current simulation result
    const hsr::SimulationResult& getResult() const { return lastResult; }
    
    // Check if simulation is running
    bool isSimulating() const { return isRunning; }
    
    // Toggle between Python and C++ engine
    void toggleEngine();
    bool isUsingPythonEngine() const { return usePythonEngine; }
    
    // Check if back navigation was requested
    bool consumeBackRequest();

private:
    // UI Layout helpers
    Rectangle headerBounds();
    Rectangle timelineBounds();
    Rectangle controlsBounds();
    Rectangle resultsBounds();
    Rectangle characterSlotBounds(size_t index);
    
    // Drawing helpers
    void drawTimeline();
    void drawCharacterSlots();
    void drawControls();
    void drawResults();
    void drawActionTooltip(const hsr::ActionEvent& action);
    
    // State management
    void resetSimulation();
    
    AssetManager& assets;
    EnemyDatabase& enemies;
    
    // Simulation state
    std::string selectedEnemyId;
    std::vector<hsr::CharacterConfig> characters;
    hsr::EnemyConfig currentEnemy;
    hsr::SimulationResult lastResult;
    
    // UI state
    bool isRunning;
    bool showResults;
    int scrollOffset;
    int hoveredActionIndex;
    float zoomLevel;
    
    // Engine selection
    bool usePythonEngine;
    
    // Character editor state
    bool isEditingCharacter;
    size_t editingIndex;
    char speedInput[16];
    char rotationInput[64];
    
    // Navigation state
    bool backRequested;
};
