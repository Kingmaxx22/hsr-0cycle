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

    // Set the full 5-slot encounter to simulate against (Sec 22.9).
    // Replaces single-enemy selection: the App builds this from the
    // Enemies screen slots, so there is exactly one enemy source of truth.
    void setEncounter(const hsr::EncounterConfig& encounter);
    
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
    // Screen box of one timeline event (zoom/scroll aware). Shared by
    // draw + hover hit-testing so the tooltip always matches the box.
    Rectangle actionBox(size_t index);
    
    // State management
    void resetSimulation();
    
    AssetManager& assets;
    EnemyDatabase& enemies;
    
    // Simulation state
    hsr::EncounterConfig currentEncounter;
    bool hasEncounter = false;
    std::vector<hsr::CharacterConfig> characters;
    hsr::EnemyConfig currentEnemy; // first encounter enemy, for display
    hsr::SimulationResult lastResult;

    size_t encounterEnemyCount() const;
    
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
