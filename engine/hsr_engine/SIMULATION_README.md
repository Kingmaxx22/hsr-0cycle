# HSR 0-Cycle Simulation Engine

Python-based Action Value (AV) simulation engine for Honkai: Star Rail 0-cycle clear validation.

## Features

- **Action Value System**: Accurate turn order calculation using `AV = 10000 / SPD`
- **Speed Breakpoints**: Calculate how many actions a unit gets within 150 AV (0-cycle) or 100 AV (1 cycle)
- **Custom Rotations**: Define precise action sequences with skill point tracking
- **Auto Simulation**: Basic AI for quick testing
- **Action Advance**: Support for -24 AV advances (Bronya), percentage advances, and instant actions
- **Enemy Integration**: Load enemies from `monsters_rules.json` with resistances, weaknesses, toughness

## Quick Start

```python
from hsr_engine import (
    simulate_0cycle,
    calculate_speed_breakpoint,
    get_speed_table,
    SimAction,
    ActionType,
    SimulationEngine
)

# Get speed breakpoint for a character
bp = calculate_speed_breakpoint(134)
print(f"SPD 134: {bp.actions_in_150_av} actions in 0-cycle")
# Output: SPD 134: 2 actions in 0-cycle

# Get full breakpoint table
for bp in get_speed_table():
    print(f"SPD {bp.spd_required}: {bp.actions_in_150_av} actions")

# Run auto simulation
units = {
    'seele': {'name': 'Seele', 'spd': 142, 'hp': 1000},
    'bronya': {'name': 'Bronya', 'spd': 134, 'hp': 800}
}

enemies = {
    'cocolia': {
        'name': 'Cocolia',
        'spd': 100,
        'hp': 80000,
        'first_turn_delay': 100.0,
        'weaknesses': ['quantum'],
        'resistances': {'physical': 0.2}
    }
}

result = simulate_0cycle(units, enemies, auto=True)
print(f"0-Cycle Success: {result.is_zero_cycle}")
print(f"Damage Dealt: {result.total_damage}")
```

## Custom Rotation Example

```python
# Define custom action sequence
actions = [
    SimAction(
        av=70.4,  # Seele's first action
        actor_id='seele',
        action_type=ActionType.SKILL,
        target_ids=['cocolia'],
        multiplier=2.2,
        sp_gain=-1,
        energy_gain=20.0
    ),
    SimAction(
        av=74.6,  # Bronya buffs Seele
        actor_id='bronya',
        action_type=ActionType.SKILL,
        target_ids=['seele'],
        advance_target='seele',  # Advance Seele
        advance_value=24.0       # -24 AV
    ),
    SimAction(
        av=74.6,  # Seele acts again immediately
        actor_id='seele',
        action_type=ActionType.SKILL,
        target_ids=['cocolia'],
        multiplier=2.2
    )
]

engine = SimulationEngine()
state = engine.setup_simulation(units, enemies)
result = engine.simulate_rotation(state, actions)

# View timeline
for record in result.actions_taken:
    print(f"AV {record.av}: {record.actor_name} -> {record.target_names}")
```

## Key Classes

### `AVCalculator`
- `calculate_av(spd)` - Convert speed to action value
- `get_breakpoint(spd, av_limit)` - Get action count for given speed
- `get_speed_breakpoints_table()` - Standard breakpoint reference table

### `SimulationEngine`
- `setup_simulation(units, enemies)` - Initialize battle state
- `auto_simulate(state)` - Run with basic AI
- `simulate_rotation(state, actions)` - Run custom action queue

### `SimAction`
Represents an action in the timeline:
- `av` - When the action occurs
- `actor_id` - Who performs it
- `action_type` - BASIC, SKILL, ULTIMATE, FOLLOW_UP
- `target_ids` - Target list
- `multiplier` - Damage multiplier
- `sp_gain` - Skill point change (-1 for skill, +1 for basic)
- `advance_target` / `advance_value` - Action advance effects

### `SimulationResult`
- `success` - Were all enemies defeated?
- `is_zero_cycle` - Cleared within 150 AV?
- `enemy_defeated_at_av` - When boss died
- `total_damage` - Total damage dealt
- `actions_taken` - List of TurnRecord objects
- `skill_points_history` - SP tracking through fight

## Speed Breakpoint Reference

| SPD | 150 AV Actions | 100 AV Actions | First Action AV |
|-----|----------------|----------------|-----------------|
| 100 | 1 | 1 | 100.0 |
| 120 | 1 | 1 | 83.3 |
| 134 | 2 | 1 | 74.6 |
| 142 | 2 | 1 | 70.4 |
| 150 | 2 | 1 | 66.7 |
| 160 | 2 | 1 | 62.5 |
| 180 | 2 | 1 | 55.6 |
| 200 | 3 | 2 | 50.0 |

**Key Breakpoints:**
- **134 SPD**: 2 actions in 0-cycle (most common DPS threshold)
- **142 SPD**: 2 actions with room for speed buffs
- **200 SPD**: 3 actions in 0-cycle (rare, requires heavy buffering)

## Integration with C++ UI

The Python engine is designed to be called from the C++ raylib UI via subprocess or embedded Python:

```cpp
// Pseudo-code for C++ integration
std::string run_simulation_json = R"({
    "units": {...},
    "enemies": [...],
    "actions": [...]
})";

// Call Python script, get JSON result back
std::string result = call_python_engine(run_simulation_json);

// Parse and display timeline in UI
auto sim_result = parse_json(result);
draw_timeline(sim_result.timeline);
```

## Running Examples

```bash
cd /workspace/engine/hsr_engine

# Speed breakpoints and auto-sim demo
PYTHONPATH=. python examples/zero_cycle_demo.py

# Basic smoke test
PYTHONPATH=. python examples/smoke_test.py

# Run tests
PYTHONPATH=. python -m pytest tests/test_engine.py
```

## Next Steps

1. **UI Integration**: Create `SimulationScreen.cpp` to visualize timelines
2. **Character Actions**: Add predefined rotations for popular characters
3. **Damage Formulas**: Integrate with existing `RuleEngine` for accurate damage
4. **Relic Optimization**: Test different speed breakpoints for relic optimization
5. **Export/Import**: Save rotation configurations as JSON

## Architecture

```
hsr_engine/
├── simulation.py      # NEW: AV system & 0-cycle simulator
├── engine.py          # Rule engine for damage/effects
├── model.py           # Data classes (Unit, Action, BattleState)
├── stats.py           # Stat calculation
├── damage.py          # Damage formulas
└── effects.py         # Effect/buff handling
```

The simulation engine uses simplified damage by default but can integrate with the full `RuleEngine` for accurate calculations including relics, light cones, and character-specific mechanics.
