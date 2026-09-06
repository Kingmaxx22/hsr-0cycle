#ifndef ENGINE_BRIDGE_H
#define ENGINE_BRIDGE_H

#include "simulation/SimulationEngine.h"
#include <string>
#include <vector>
#include <map>

namespace hsr {

// Bridge class to interface between C++ UI and Python simulation engine
class EngineBridge {
public:
    EngineBridge();
    ~EngineBridge();

    // Initialize Python interpreter and load simulation module
    bool initialize();
    
    // Cleanup Python resources
    void shutdown();
    
    // Check if bridge is ready
    bool isReady() const;

    // Run simulation using Python engine
    SimulationResult runPythonSimulation(
        const std::vector<CharacterConfig>& characters,
        const EnemyConfig& enemy,
        int avLimit = 15000
    );

    // Get speed breakpoints from Python engine
    std::vector<SimulationEngine::SpeedBreakpoint> getPythonBreakpoints(
        int baseSpeed, 
        int avLimit = 15000
    );

    // Convert JSON string to CharacterConfig
    static CharacterConfig parseCharacterFromJson(const std::string& json);
    
    // Convert JSON string to EnemyConfig
    static EnemyConfig parseEnemyFromJson(const std::string& json);

private:
    bool m_initialized;
    void* m_pPythonState; // Opaque pointer to Python state
    
    // Helper methods
    std::string buildPythonCharacterDict(const CharacterConfig& config);
    std::string buildPythonEnemyDict(const EnemyConfig& config);
    SimulationResult parsePythonResult(const std::string& jsonResult);
};

} // namespace hsr

#endif // ENGINE_BRIDGE_H
