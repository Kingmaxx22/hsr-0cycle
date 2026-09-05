from __future__ import annotations
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Set

@dataclass
class Duration:
    kind: str = "permanent"
    value: Optional[int] = None

@dataclass
class Condition:
    kind: str
    args: Dict[str, Any] = field(default_factory=dict)

@dataclass
class Effect:
    id: str
    kind: str
    source: str
    target: str = "self"
    stat: Optional[str] = None
    value: Any = None
    value_type: str = "flat"
    duration: Duration = field(default_factory=Duration)
    condition: Optional[Condition] = None
    trigger: Optional[str] = None
    stacks: int = 1
    max_stacks: Optional[int] = None
    tags: Set[str] = field(default_factory=set)
    metadata: Dict[str, Any] = field(default_factory=dict)

@dataclass
class Action:
    id: str
    actor_id: str
    kind: str
    target_ids: List[str]
    multiplier: float = 0.0
    scaling_stat: str = "atk"
    damage_type: Optional[str] = None
    energy_gain: float = 0.0
    energy_cost: float = 0.0
    sp_cost: int = 0
    break_value: float = 0.0
    tags: Set[str] = field(default_factory=set)
    metadata: Dict[str, Any] = field(default_factory=dict)

@dataclass
class Unit:
    id: str
    name: str
    level: int = 80
    base_stats: Dict[str, float] = field(default_factory=dict)
    stats: Dict[str, float] = field(default_factory=dict)
    energy: float = 0.0
    max_energy: float = 100.0
    hp: float = 1.0
    max_hp: float = 1.0
    effects: List[Effect] = field(default_factory=list)
    states: Dict[str, Any] = field(default_factory=dict)
    tags: Set[str] = field(default_factory=set)
    team_id: str = "team_0"

@dataclass
class BattleState:
    units: Dict[str, Unit]
    skill_points: int = 3
    max_skill_points: int = 5
    time: float = 0.0
    cycle_limit_av: float = 150.0
    events: List[Dict[str, Any]] = field(default_factory=list)
    defeated: Set[str] = field(default_factory=set)

    def living(self, team_id=None):
        result = [u for u in self.units.values()
                  if u.id not in self.defeated and u.hp > 0]
        return [u for u in result if team_id is None or u.team_id == team_id]
