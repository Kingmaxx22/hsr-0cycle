from dataclasses import dataclass
from .stats import StatEngine

@dataclass
class DamageResult:
    raw: float
    final: float
    crit: bool
    defense_multiplier: float
    resistance_multiplier: float
    vulnerability_multiplier: float

class DamageEngine:
    def __init__(self):
        self.stats = StatEngine()

    def defense_multiplier(self, attacker_level, target_def,
                           def_reduction=0, def_ignore=0, def_pen=0):
        effective = target_def * max(0.0, 1.0-def_reduction)
        effective *= max(0.0, 1.0-def_ignore)
        effective = max(0.0, effective-def_pen)
        return (attacker_level+20.0)/((attacker_level+20.0)+effective)

    def resistance_multiplier(self, target, damage_type):
        res = target.states.get("elemental_res", {}).get(damage_type, 0.0)
        if damage_type in target.states.get("weaknesses", set()):
            res = 0.0
        return max(0.0, 1.0-float(res))

    def calculate(self, attacker, target, action, state, crit=False):
        a = self.stats.calculate(attacker, state, target)
        t = self.stats.calculate(target, state, attacker)
        raw = float(a.get(action.scaling_stat,0)) * float(action.multiplier)
        if crit:
            raw *= 1.0 + float(a.get("crit_dmg",.50))

        bonus = 1.0 + float(a.get("damage",0))
        bonus *= 1.0 + float(attacker.states.get("damage_bonus",0))
        vulnerability = 1.0 + float(target.states.get("vulnerability",0))

        defense = self.defense_multiplier(
            attacker.level, float(t.get("def",0)),
            float(target.states.get("def_reduction",0)),
            float(attacker.states.get("def_ignore",0)),
            float(attacker.states.get("def_pen",0)))
        resistance = self.resistance_multiplier(
            target, action.damage_type or "other")
        return DamageResult(raw, raw*bonus*defense*resistance*vulnerability,
                            crit, defense, resistance, vulnerability)
