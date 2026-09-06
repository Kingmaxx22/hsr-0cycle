"""
HSR 0-Cycle Action Value (AV) Simulation Engine

This module provides turn-order simulation using Action Value (AV) mechanics
for Honkai: Star Rail combat. It calculates speed breakpoints, action order,
and validates 0-cycle clears (defeating enemies within 150 AV).

Key Concepts:
- Action Value (AV): The "time" until a unit's next action (lower = faster)
- Formula: AV = 10000 / SPD
- 0-Cycle Clear: Defeat all enemies before AV 150
- Speed Breakpoints: SPD thresholds for specific action counts
"""

from __future__ import annotations
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Set, Tuple
from enum import Enum
import heapq


class ActionType(Enum):
    """Types of actions in combat"""
    BASIC = "basic"
    SKILL = "skill"
    ULTIMATE = "ultimate"
    FOLLOW_UP = "follow_up"
    DOT_TICK = "dot_tick"
    SUMMON = "summon"


@dataclass
class SimAction:
    """Represents an action in the simulation timeline"""
    av: float  # Action Value when this occurs
    actor_id: str
    action_type: ActionType
    target_ids: List[str]
    multiplier: float = 1.0
    damage_type: Optional[str] = None
    sp_gain: int = 0  # Skill points gained (negative = cost)
    energy_gain: float = 0.0
    advance_target: Optional[str] = None  # ID of unit to advance
    advance_value: float = 0.0  # How much to advance (in AV or %)
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def __lt__(self, other):
        return self.av < other.av


@dataclass
class TurnRecord:
    """Records what happened on a specific turn"""
    av: float
    actor_id: str
    actor_name: str
    action_type: str
    target_names: List[str]
    damage_dealt: float = 0.0
    sp_change: int = 0
    energy_gained: float = 0.0
    notes: str = ""


@dataclass
class SpeedBreakpoint:
    """Represents a speed breakpoint for action counting"""
    spd_required: float
    actions_in_150_av: int
    actions_in_cycle: int  # Actions per cycle (100 AV)
    first_action_av: float


@dataclass 
class SimulationResult:
    """Complete results from a 0-cycle simulation"""
    success: bool  # True if 0-cycle clear achieved
    total_damage: float
    enemy_defeated_at_av: Optional[float]
    actions_taken: List[TurnRecord]
    timeline: List[SimAction]
    skill_points_history: List[Tuple[float, int]]  # (AV, SP count)
    speed_breakpoints: Dict[str, SpeedBreakpoint]
    notes: List[str] = field(default_factory=list)
    
    @property
    def is_zero_cycle(self) -> bool:
        return self.success and (self.enemy_defeated_at_av or 999) <= 150.0


class AVCalculator:
    """
    Calculates Action Values and speed breakpoints
    
    Core formula: AV = 10000 / SPD
    - Lower AV = faster action
    - Initial delay may apply (e.g., first_turn_delay for enemies)
    """
    
    @staticmethod
    def calculate_av(spd: float) -> float:
        """Calculate Action Value from speed"""
        if spd <= 0:
            return float('inf')
        return 10000.0 / spd
    
    @staticmethod
    def calculate_spd_for_av(av: float) -> float:
        """Calculate required SPD for a target AV"""
        if av <= 0:
            return float('inf')
        return 10000.0 / av
    
    @staticmethod
    def get_breakpoint(unit_spd: float, av_limit: float = 150.0) -> SpeedBreakpoint:
        """
        Calculate how many actions a unit gets within AV limit
        
        Args:
            unit_spd: Unit's speed stat
            av_limit: Maximum AV to consider (default 150 for 0-cycle)
            
        Returns:
            SpeedBreakpoint with action counts
        """
        if unit_spd <= 0:
            return SpeedBreakpoint(
                spd_required=unit_spd,
                actions_in_150_av=0,
                actions_in_cycle=0,
                first_action_av=float('inf')
            )
        
        first_av = AVCalculator.calculate_av(unit_spd)
        
        # Count actions within 150 AV
        actions_150 = 0
        current_av = first_av
        while current_av <= av_limit:
            actions_150 += 1
            current_av += first_av
        
        # Count actions within 100 AV (one cycle)
        actions_cycle = 0
        current_av = first_av
        while current_av <= 100.0:
            actions_cycle += 1
            current_av += first_av
        
        return SpeedBreakpoint(
            spd_required=unit_spd,
            actions_in_150_av=actions_150,
            actions_in_cycle=actions_cycle,
            first_action_av=first_av
        )
    
    @staticmethod
    def get_speed_breakpoints_table() -> List[SpeedBreakpoint]:
        """
        Generate a table of important speed breakpoints
        
        Returns common SPD thresholds and their action counts
        """
        # Key SPD values for HSR
        key_spd_values = [
            100, 105, 110, 115, 120, 125, 130, 134, 138, 140,
            145, 150, 155, 160, 165, 170, 175, 180, 190, 200
        ]
        
        return [
            AVCalculator.get_breakpoint(spd)
            for spd in sorted(key_spd_values)
        ]


class ActionAdvancer:
    """
    Handles action advancement mechanics
    
    Supports:
    - Flat AV reduction (e.g., -24 AV from Bronya skill)
    - Percentage advance (e.g., "advance forward 25%")
    - Instant action ("immediately take action")
    """
    
    @staticmethod
    def apply_flat_advance(current_av: float, reduction: float, 
                          min_av: float = 0.0) -> float:
        """
        Reduce AV by a flat amount
        
        Args:
            current_av: Current action value
            reduction: Amount to reduce (e.g., 24 for -24 AV)
            min_av: Minimum AV after reduction (default 0)
        """
        return max(min_av, current_av - reduction)
    
    @staticmethod
    def apply_percent_advance(current_av: float, percent: float,
                             full_cycle_av: float) -> float:
        """
        Advance by percentage of full action
        
        Args:
            current_av: Current position in action order
            percent: Percentage to advance (e.g., 0.25 for 25%)
            full_cycle_av: Full action AV (10000/SPD)
        """
        advance_amount = full_cycle_av * percent
        return max(0.0, current_av - advance_amount)
    
    @staticmethod
    def instant_action() -> float:
        """Return AV for instant action"""
        return 0.0


class SimulationEngine:
    """
    Main 0-cycle combat simulation engine
    
    Simulates turn order and damage output to validate 0-cycle clears
    """
    
    def __init__(self, rule_engine=None):
        """
        Initialize simulation engine
        
        Args:
            rule_engine: Optional RuleEngine instance for damage calculation
                        If None, uses simplified damage model
        """
        self.rule_engine = rule_engine
        self.av_calc = AVCalculator()
        
    def setup_simulation(self, units: Dict[str, Any], 
                        enemies: Dict[str, Any],
                        initial_sp: int = 3,
                        start_from_zero: bool = False) -> 'SimulationState':
        """
        Initialize simulation state
        
        Args:
            units: Dict of unit data (characters)
            enemies: Dict of enemy data
            initial_sp: Starting skill points
            start_from_zero: If True, all units start at AV 0 (simulates 
                           continuing from previous cycle)
        """
        state = SimulationState(
            skill_points=initial_sp,
            max_skill_points=5,
            av_limit=150.0
        )
        
        # Add units to simulation
        for unit_id, unit_data in units.items():
            spd = unit_data.get('spd', 100.0)
            initial_av = 0.0 if start_from_zero else self.av_calc.calculate_av(spd)
            
            sim_unit = SimulationUnit(
                id=unit_id,
                name=unit_data.get('name', unit_id),
                spd=spd,
                hp=unit_data.get('hp', 1000.0),
                max_hp=unit_data.get('max_hp', 1000.0),
                energy=unit_data.get('energy', 0.0),
                max_energy=unit_data.get('max_energy', 100.0),
                current_av=initial_av,
                full_cycle_av=self.av_calc.calculate_av(spd),
                is_enemy=False
            )
            state.units[unit_id] = sim_unit
            
        # Add enemies
        for enemy_id, enemy_data in enemies.items():
            spd = enemy_data.get('spd', 100.0)
            initial_av = enemy_data.get('first_turn_delay', 
                                       self.av_calc.calculate_av(spd))
            
            sim_enemy = SimulationUnit(
                id=enemy_id,
                name=enemy_data.get('name', enemy_id),
                spd=spd,
                hp=enemy_data.get('hp', 10000.0),
                max_hp=enemy_data.get('max_hp', 10000.0),
                current_av=initial_av,
                full_cycle_av=self.av_calc.calculate_av(spd),
                is_enemy=True,
                toughness=enemy_data.get('toughness', 100.0),
                weaknesses=set(enemy_data.get('weaknesses', [])),
                resistances=enemy_data.get('resistances', {})
            )
            state.units[enemy_id] = sim_enemy
            
        return state
    
    def get_next_actor(self, state: 'SimulationState') -> Optional[SimulationUnit]:
        """Get the unit with lowest current AV (next to act)"""
        living = [u for u in state.units.values() 
                 if u.hp > 0 and u.id not in state.defeated]
        if not living:
            return None
        return min(living, key=lambda u: u.current_av)
    
    def advance_time(self, state: 'SimulationState', target_av: float):
        """
        Advance all units' AV by the difference
        
        This moves time forward to when the next unit acts
        """
        for unit in state.units.values():
            if unit.hp > 0 and unit.id not in state.defeated:
                unit.current_av -= target_av
    
    def execute_action(self, state: 'SimulationState', 
                      action: SimAction) -> TurnRecord:
        """
        Execute an action in the simulation
        
        Updates HP, SP, energy, and applies effects
        """
        actor = state.units.get(action.actor_id)
        if not actor:
            return TurnRecord(
                av=action.av,
                actor_id=action.actor_id,
                actor_name="Unknown",
                action_type=action.action_type.value,
                target_names=[],
                notes="Actor not found"
            )
        
        # Process targets
        target_names = []
        total_damage = 0.0
        
        for target_id in action.target_ids:
            target = state.units.get(target_id)
            if target and target.hp > 0:
                target_names.append(target.name)
                
                # Apply damage if action has multiplier
                if action.multiplier > 0:
                    damage = self._calculate_simple_damage(
                        actor, target, action
                    )
                    target.hp = max(0.0, target.hp - damage)
                    total_damage += damage
                    
                    if target.hp <= 0:
                        state.defeated.add(target_id)
                        
                        if target.is_enemy:
                            state.enemy_defeated_at_av = action.av
                
                # Apply toughness damage
                if action.metadata.get('toughness_dmg', 0) > 0 and target.toughness:
                    target.toughness = max(0.0, 
                                          target.toughness - action.metadata['toughness_dmg'])
        
        # Update skill points
        old_sp = state.skill_points
        state.skill_points = max(0, min(state.max_skill_points,
                                       state.skill_points + action.sp_gain))
        
        # Update energy
        actor.energy = min(actor.max_energy, actor.energy + action.energy_gain)
        
        # Handle action advances
        if action.advance_target and action.advance_value > 0:
            advancer = state.units.get(action.advance_target)
            if advancer and advancer.hp > 0:
                advancer.current_av = ActionAdvancer.apply_flat_advance(
                    advancer.current_av,
                    action.advance_value
                )
        
        # Reset actor's AV for next turn
        actor.current_av += actor.full_cycle_av
        
        return TurnRecord(
            av=action.av,
            actor_id=actor.id,
            actor_name=actor.name,
            action_type=action.action_type.value,
            target_names=target_names,
            damage_dealt=total_damage,
            sp_change=state.skill_points - old_sp,
            energy_gained=action.energy_gain
        )
    
    def _calculate_simple_damage(self, attacker: SimulationUnit,
                                target: SimulationUnit,
                                action: SimAction) -> float:
        """
        Simplified damage calculation for simulation
        
        Uses basic formula without full stat calculations
        Can be replaced with RuleEngine integration
        """
        # Base damage from ATK * multiplier
        base_atk = 800.0  # Assumed average ATK
        base_damage = base_atk * action.multiplier
        
        # Crit (assume 50% crit rate, 150% crit dmg for simplicity)
        import random
        is_crit = random.random() < 0.5
        if is_crit:
            base_damage *= 2.5
        
        # Defense reduction (simplified)
        def_ratio = 1150.0 / (1150.0 + 100)  # Target DEF vs Attacker level
        base_damage *= def_ratio
        
        # Resistance
        res = target.resistances.get(action.damage_type or 'physical', 0.0)
        if action.damage_type in target.weaknesses:
            res = 0.0
        base_damage *= (1.0 - res)
        
        # Vulnerability
        vuln = 1.0 + target.states.get('vulnerability', 0.0)
        base_damage *= vuln
        
        return base_damage
    
    def simulate_rotation(self, state: 'SimulationState',
                         action_queue: List[SimAction]) -> SimulationResult:
        """
        Simulate a predefined action rotation
        
        Args:
            state: Simulation state
            action_queue: Ordered list of actions to execute
            
        Returns:
            SimulationResult with full timeline and metrics
        """
        # Sort actions by AV
        queue = sorted(action_queue, key=lambda a: a.av)
        
        timeline = []
        turn_records = []
        sp_history = [(0.0, state.skill_points)]
        total_damage = 0.0
        
        for action in queue:
            # Check if we've exceeded AV limit
            if action.av > state.av_limit:
                break
                
            # Check if all enemies are defeated
            living_enemies = [u for u in state.units.values()
                            if u.is_enemy and u.hp > 0]
            if not living_enemies:
                break
            
            # Execute action
            record = self.execute_action(state, action)
            timeline.append(action)
            turn_records.append(record)
            total_damage += record.damage_dealt
            sp_history.append((action.av, state.skill_points))
        
        # Calculate speed breakpoints for all units
        speed_bps = {}
        for unit in state.units.values():
            if not unit.is_enemy:
                speed_bps[unit.id] = self.av_calc.get_breakpoint(unit.spd)
        
        enemy_defeated = len([u for u in state.units.values() 
                             if u.is_enemy and u.id in state.defeated]) == \
                        len([u for u in state.units.values() if u.is_enemy])
        
        return SimulationResult(
            success=enemy_defeated,
            total_damage=total_damage,
            enemy_defeated_at_av=state.enemy_defeated_at_av,
            actions_taken=turn_records,
            timeline=timeline,
            skill_points_history=sp_history,
            speed_breakpoints=speed_bps,
            notes=[]
        )
    
    def auto_simulate(self, state: 'SimulationState',
                     max_actions: int = 20) -> SimulationResult:
        """
        Automatically simulate combat with basic AI
        
        Units use skills when they have enough SP, otherwise basic attacks
        
        Args:
            state: Simulation state
            max_actions: Maximum number of actions to simulate
            
        Returns:
            SimulationResult
        """
        timeline = []
        turn_records = []
        sp_history = [(0.0, state.skill_points)]
        total_damage = 0.0
        action_count = 0
        
        while action_count < max_actions:
            # Get next actor
            actor = self.get_next_actor(state)
            if not actor:
                break
            
            current_av = actor.current_av
            
            # Check AV limit
            if current_av > state.av_limit:
                break
            
            # Check if enemies remain
            living_enemies = [u for u in state.units.values()
                            if u.is_enemy and u.hp > 0]
            if not living_enemies:
                break
            
            # Select target (lowest HP enemy)
            target = min(living_enemies, key=lambda e: e.hp)
            
            # Decide action type
            if actor.is_enemy:
                # Enemy uses basic attack
                action_type = ActionType.BASIC
                sp_gain = 0
                energy_gain = 10.0
                multiplier = 1.0
            else:
                # Character logic
                if state.skill_points >= 1:
                    action_type = ActionType.SKILL
                    sp_gain = -1
                    energy_gain = 20.0
                    multiplier = 1.5
                else:
                    action_type = ActionType.BASIC
                    sp_gain = 1
                    energy_gain = 10.0
                    multiplier = 1.0
            
            action = SimAction(
                av=current_av,
                actor_id=actor.id,
                action_type=action_type,
                target_ids=[target.id],
                multiplier=multiplier,
                sp_gain=sp_gain,
                energy_gain=energy_gain
            )
            
            # Execute
            record = self.execute_action(state, action)
            timeline.append(action)
            turn_records.append(record)
            total_damage += record.damage_dealt
            sp_history.append((current_av, state.skill_points))
            action_count += 1
        
        # Calculate results
        enemy_defeated = len([u for u in state.units.values() 
                             if u.is_enemy and u.id in state.defeated]) == \
                        len([u for u in state.units.values() if u.is_enemy])
        
        speed_bps = {}
        for unit in state.units.values():
            if not unit.is_enemy:
                speed_bps[unit.id] = self.av_calc.get_breakpoint(unit.spd)
        
        return SimulationResult(
            success=enemy_defeated,
            total_damage=total_damage,
            enemy_defeated_at_av=state.enemy_defeated_at_av,
            actions_taken=turn_records,
            timeline=timeline,
            skill_points_history=sp_history,
            speed_breakpoints=speed_bps
        )


@dataclass
class SimulationUnit:
    """Unit representation for simulation"""
    id: str
    name: str
    spd: float
    hp: float
    max_hp: float
    current_av: float
    full_cycle_av: float
    is_enemy: bool = False
    energy: float = 0.0
    max_energy: float = 100.0
    toughness: Optional[float] = None
    weaknesses: Set[str] = field(default_factory=set)
    resistances: Dict[str, float] = field(default_factory=dict)
    states: Dict[str, Any] = field(default_factory=dict)


@dataclass
class SimulationState:
    """Current state of the simulation"""
    units: Dict[str, SimulationUnit] = field(default_factory=dict)
    skill_points: int = 3
    max_skill_points: int = 5
    av_limit: float = 150.0
    defeated: Set[str] = field(default_factory=set)
    enemy_defeated_at_av: Optional[float] = None


# Convenience functions for Python API
def calculate_speed_breakpoint(spd: float, av_limit: float = 150.0) -> SpeedBreakpoint:
    """Calculate actions for a given speed"""
    return AVCalculator.get_breakpoint(spd, av_limit)


def get_speed_table() -> List[SpeedBreakpoint]:
    """Get standard speed breakpoint table"""
    return AVCalculator.get_speed_breakpoints_table()


def simulate_0cycle(units: Dict[str, Any],
                   enemies: Dict[str, Any],
                   actions: Optional[List[SimAction]] = None,
                   auto: bool = True) -> SimulationResult:
    """
    Quick 0-cycle simulation
    
    Args:
        units: Character data dict
        enemies: Enemy data dict  
        actions: Optional predefined action queue
        auto: If True, use auto-simulation; if False, use provided actions
        
    Returns:
        SimulationResult
    """
    engine = SimulationEngine()
    state = engine.setup_simulation(units, enemies)
    
    if auto or actions is None:
        return engine.auto_simulate(state)
    else:
        return engine.simulate_rotation(state, actions)
