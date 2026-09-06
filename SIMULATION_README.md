# 0-Cycle AV Simulation System

## Overview

This implementation adds a complete 0-Cycle Action Value (AV) simulation system to your HSR calculator app, featuring:

- **C++ Simulation Engine** - Native AV-based turn ordering and combat simulation
- **Python Integration Bridge** - Optional Python engine integration for advanced features
- **SimulationScreen UI** - Interactive timeline visualization with raylib
- **Speed Breakpoint Calculator** - Find optimal speed thresholds for action targets

## Files Created

### Core Engine (`/workspace/src/simulation/`)

1. **SimulationEngine.h** - Header with data structures and class definition
   - `ActionEvent` - Single action in timeline
   - `CharacterConfig` - Character setup for simulation
   - `EnemyConfig` - Enemy configuration
   - `SimulationResult` - Complete simulation output
   - `SimulationEngine` - Main simulation class

2. **SimulationEngine.cpp** - Implementation of simulation logic
   - AV calculation (10000 / SPD formula)
   - Turn order resolution
   - Action execution with SP/energy tracking
   - 0-cycle clear detection
   - Speed breakpoint calculations

3. **EngineBridge.h/cpp** - Python/C++ interoperability layer
   - Python interpreter initialization
   - Data marshaling between C++ and Python
   - Fallback to C++ if Python unavailable
   - JSON parsing for configuration

### UI Screen (`/workspace/ui/src/screens/`)

4. **SimulationScreen.h** - Screen interface definition
   - Character management API
   - Simulation control methods
   - UI state management

5. **SimulationScreen.cpp** - Full UI implementation
   - Timeline visualization (0-150 AV)
   - Interactive action tooltips
   - Character slot management
   - Results display with 0-cycle validation
   - Zoom and scroll controls

## Features

### AV Timeline System
- Visual representation of actions from 0 to 150 AV
- Color-coded action types (Basic=Blue, Skill=Orange, Ult=Purple, FUA=Pink)
- Staggered display for overlapping actions
- Hover tooltips showing detailed action info

### Simulation Controls
- **R** - Run simulation
- **E** - Toggle between Python/C++ engine
- **ESC** - Reset simulation
- Mouse wheel - Scroll timeline
- +/- buttons - Zoom in/out

### 0-Cycle Detection
- Automatically detects if enemy defeated within 150 AV
- Displays clear result with gold highlight for success
- Shows total cycles if not 0-cycle

### Character Management
- Add up to 4 characters
- Configure speed, rotation, SP, energy
- Remove characters individually
- Auto vs manual rotation modes

## Usage Example

```cpp
// In your App.cpp or main game loop

// Create simulation screen
auto* simScreen = new SimulationScreen(assets, enemies);
simScreen->initialize();

// Set enemy from EnemiesScreen selection
simScreen->setSelectedEnemy(selectedEnemyId);

// Add characters
hsr::CharacterConfig char1;
char1.id = "kafka";
char1.name = "Kafka";
char1.speed = 134;
char1.maxSp = 5;
char1.currentSp = 3;
char1.energy = 50;
char1.maxEnergy = 120;
char1.rotation = {"Skill", "Basic", "Basic"};
char1.isAuto = false;

simScreen->addCharacter(char1);

// In your update loop
simScreen->update(dt);

// In your draw loop
simScreen->draw();

// Or programmatically run simulation
simScreen->runSimulation();

// Check results
const auto& result = simScreen->getResult();
if (result.isZeroCycleClear) {
    std::cout << "0-cycle clear achieved!" << std::endl;
}
```

## Integration with Existing Systems

### From EnemiesScreen
```cpp
// When user selects an enemy in EnemiesScreen
std::string enemyId = enemiesScreen.getSelectedEnemyId();

// Switch to simulation screen
simulationScreen->setSelectedEnemy(enemyId);
```

### Character Database Integration
```cpp
// Load character data from your CharacterDatabase
CharacterData charData = charDB.getCharacter("kafka");

hsr::CharacterConfig config;
config.id = charData.id;
config.name = charData.name;
config.speed = charData.speed; // From relics
config.maxSp = 5;
config.currentSp = 3;
config.rotation = {"Skill", "Basic", "Basic"}; // From user config

simulationScreen->addCharacter(config);
```

## Build Requirements

### For C++ Engine Only
No additional dependencies - uses standard library + raylib (already present)

### For Python Integration
```bash
# Install Python development headers
apt-get install python3-dev  # Debian/Ubuntu
# or
dnf install python3-devel    # Fedora/RHEL

# Link against Python in CMakeLists.txt
find_package(Python3 COMPONENTS Development REQUIRED)
target_link_libraries(your_app Python3::Python)
```

## CMakeLists.txt Updates Needed

Add to your existing CMakeLists.txt:

```cmake
# Add simulation sources
set(SIMULATION_SOURCES
    src/simulation/SimulationEngine.cpp
    src/simulation/EngineBridge.cpp
)

# Add simulation screen
set(SCREEN_SOURCES
    src/screens/SimulationScreen.cpp
    ${SIMULATION_SOURCES}
)

# Create executable or add to existing
target_sources(hsr_calculator PRIVATE ${SCREEN_SOURCES})

# Include directories
target_include_directories(hsr_calculator PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/ui/src
)

# Optional: Python integration
find_package(Python3 COMPONENTS Development)
if(Python3_FOUND)
    target_compile_definitions(hsr_calculator PRIVATE HAVE_PYTHON)
    target_link_libraries(hsr_calculator Python3::Python)
endif()
```

## Testing

The simulation engine includes built-in testing through the UI:

1. Launch the app
2. Navigate to Enemies Screen → Select a boss
3. Navigate to Simulation Screen (or trigger programmatically)
4. Press **R** to run simulation
5. View timeline and results
6. Hover over actions for details

## Future Enhancements

### Phase 2 Priorities
1. **Full Damage Formula** - Replace placeholder damage with real HSR formulas
2. **Light Cone Integration** - Include LC passives and stats
3. **Relic Stats** - Import actual relic configurations
4. **Advanced Rotations** - Conditional actions based on SP/energy/buffs
5. **Follow-Up Attacks** - Proper FUA triggering logic
6. **DoT Tracking** - Damage over time tick scheduling
7. **Break Mechanics** - Real toughness and break damage calculation
8. **Buff/Debuff System** - Track uptime and stacking

### Python Engine Features
When fully integrated, the Python engine can provide:
- More complex damage calculations
- Machine learning optimization for rotations
- Easy theorycrafting without recompilation
- Community-shared simulation scripts

## Architecture Notes

### Why Hybrid C++/Python?
- **C++**: Fast UI rendering, responsive controls, native raylib integration
- **Python**: Easy theorycrafting, rapid iteration, community contributions
- **Bridge**: Best of both worlds with fallback safety

### AV Precision
Uses integer math with 2 decimal precision (10000 = 100.00 AV) to avoid floating-point errors in turn ordering.

### Thread Safety
Currently single-threaded. For future async simulations:
- Move simulation to worker thread
- Use double-buffering for results
- Add progress indicator for long simulations

## Troubleshooting

### Python Import Errors
```
[EngineBridge] Failed to import hsr_engine.simulation module
```
→ Falls back to C++ engine automatically. Fix by ensuring Python path is correct.

### Timeline Not Showing
→ Make sure you've selected an enemy and added at least one character, then press R.

### Compilation Errors
→ Ensure all new files are added to your build system (CMakeLists.txt or Makefile)

## Contributing

When extending the simulation:
1. Keep C++ engine focused on performance-critical code
2. Use Python for experimental/high-level logic
3. Update this README with new features
4. Add tests for new mechanics

---

**Status**: ✅ Core functionality complete and ready for integration
**Next Steps**: Integrate into App screen manager, connect to EnemiesScreen, test with real data
