"""
Example: 0-Cycle Simulation with the HSR Engine

Demonstrates:
- Speed breakpoint calculations
- Turn order simulation
- Custom action rotations
- 0-cycle validation
"""

from hsr_engine import (
    simulate_0cycle,
    calculate_speed_breakpoint,
    get_speed_table,
    SimAction,
    ActionType,
    SimulationEngine
)


def demo_speed_breakpoints():
    """Show speed breakpoint table"""
    print("=" * 70)
    print("SPEED BREAKPOINT TABLE")
    print("=" * 70)
    print(f"{'SPD':>6} | {'150 AV Actions':>14} | {'Cycle Actions':>13} | {'First Action':>12}")
    print("-" * 70)
    
    for bp in get_speed_table():
        print(f"{bp.spd_required:>6.0f} | {bp.actions_in_150_av:>14} | {bp.actions_in_cycle:>13} | {bp.first_action_av:>11.1f} AV")
    
    print()


def demo_custom_rotation():
    """Demonstrate custom action rotation simulation"""
    print("=" * 70)
    print("CUSTOM ROTATION SIMULATION")
    print("=" * 70)
    
    # Define team composition
    units = {
        'seele': {
            'name': 'Seele',
            'spd': 142,  # 2 actions in 150 AV
            'hp': 1000,
            'max_hp': 1000
        },
        'bronya': {
            'name': 'Bronya',
            'spd': 134,  # 2 actions in 150 AV  
            'hp': 800,
            'max_hp': 800
        }
    }
    
    # Define enemy
    enemies = {
        'cocolia': {
            'name': 'Cocolia',
            'spd': 100,
            'hp': 80000,
            'max_hp': 80000,
            'first_turn_delay': 100.0,
            'weaknesses': ['quantum'],
            'resistances': {'physical': 0.2, 'fire': 0.2}
        }
    }
    
    # Create custom action queue
    # Format: Seele skill -> Bronya skill on Seele -> Seele skill again
    actions = [
        # Seele acts first at 70.4 AV (10000/142)
        SimAction(
            av=70.4,
            actor_id='seele',
            action_type=ActionType.SKILL,
            target_ids=['cocolia'],
            multiplier=2.2,
            sp_gain=-1,
            energy_gain=20.0,
            damage_type='quantum'
        ),
        # Bronya acts at 74.6 AV (10000/134)
        SimAction(
            av=74.6,
            actor_id='bronya',
            action_type=ActionType.SKILL,
            target_ids=['seele'],  # Buff Seele
            multiplier=1.0,
            sp_gain=-1,
            energy_gain=15.0,
            advance_target='seele',  # Advance Seele
            advance_value=24.0  # -24 AV advance
        ),
        # Seele acts again immediately due to advance (70.4 - 24 = 46.4, but can't go below 0)
        SimAction(
            av=74.6,  # Acts immediately after Bronya
            actor_id='seele',
            action_type=ActionType.SKILL,
            target_ids=['cocolia'],
            multiplier=2.2,
            sp_gain=-1,
            energy_gain=20.0,
            damage_type='quantum'
        ),
        # Seele's normal second turn at 140.8 AV
        SimAction(
            av=140.8,
            actor_id='seele',
            action_type=ActionType.SKILL,
            target_ids=['cocolia'],
            multiplier=2.2,
            sp_gain=-1,
            energy_gain=20.0,
            damage_type='quantum'
        )
    ]
    
    engine = SimulationEngine()
    state = engine.setup_simulation(units, enemies, initial_sp=3)
    result = engine.simulate_rotation(state, actions)
    
    print(f"\nTeam: Seele (SPD 142), Bronya (SPD 134)")
    print(f"Enemy: Cocolia (80k HP)")
    print(f"\nResult: {'✓ 0-CYCLE CLEAR' if result.is_zero_cycle else '✗ No clear'}")
    print(f"Enemy defeated at: {result.enemy_defeated_at_av or 'Not defeated'} AV")
    print(f"Total damage dealt: {result.total_damage:,.0f}")
    
    print("\nAction Timeline:")
    print(f"{'AV':>8} | {'Actor':>12} | {'Action':>10} | {'Target':>12} | {'DMG':>10} | {'SP Change':>9}")
    print("-" * 70)
    
    for record in result.actions_taken:
        targets = ', '.join(record.target_names) if record.target_names else '-'
        print(f"{record.av:>8.1f} | {record.actor_name:>12} | {record.action_type:>10} | {targets:>12} | {record.damage_dealt:>10.0f} | {record.sp_change:>+9}")
    
    print()


def demo_auto_simulation():
    """Demonstrate automatic simulation with basic AI"""
    print("=" * 70)
    print("AUTO SIMULATION (Basic AI)")
    print("=" * 70)
    
    units = {
        'jingliu': {'name': 'Jingliu', 'spd': 138, 'hp': 1200, 'max_hp': 1200},
        'tingyun': {'name': 'Tingyun', 'spd': 125, 'hp': 700, 'max_hp': 700}
    }
    
    enemies = {
        'swarm_king': {
            'name': 'Swarm King',
            'spd': 105,
            'hp': 120000,
            'max_hp': 120000,
            'first_turn_delay': 100.0,
            'weaknesses': ['fire', 'imaginary'],
            'resistances': {}
        }
    }
    
    result = simulate_0cycle(units, enemies, auto=True)
    
    print(f"\nTeam: Jingliu (SPD 138), Tingyun (SPD 125)")
    print(f"Enemy: Swarm King (120k HP)")
    print(f"\nResult: {'✓ 0-CYCLE CLEAR' if result.is_zero_cycle else '✗ No clear'}")
    print(f"Enemy defeated at: {result.enemy_defeated_at_av or 'Not defeated'} AV")
    print(f"Total damage: {result.total_damage:,.0f}")
    
    print("\nTurn Order (first 8 actions):")
    for i, record in enumerate(result.actions_taken[:8]):
        print(f"  {i+1}. AV {record.av:6.1f}: {record.actor_name} uses {record.action_type} -> {record.target_names[0] if record.target_names else '-'} ({record.damage_dealt:.0f} dmg)")
    
    print()


def main():
    print("\n" + "=" * 70)
    print("HSR 0-CYCLE SIMULATOR - PYTHON ENGINE DEMO")
    print("=" * 70 + "\n")
    
    demo_speed_breakpoints()
    demo_custom_rotation()
    demo_auto_simulation()
    
    print("=" * 70)
    print("Demo complete! The Python engine is ready for integration.")
    print("=" * 70)


if __name__ == "__main__":
    main()
