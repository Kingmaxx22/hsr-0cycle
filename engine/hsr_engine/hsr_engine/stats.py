from collections import defaultdict
from .effects import EffectEngine

class StatEngine:
    def __init__(self):
        self.effects = EffectEngine()

    def calculate(self, unit, state, target=None):
        result = defaultdict(float)
        for k,v in unit.base_stats.items():
            result[k] = float(v)

        pct = defaultdict(float)
        mult = defaultdict(float)
        for e in self.effects.collect(unit, target, state):
            if e.kind != "stat_modifier" or not e.stat: continue
            if e.value_type == "percent":
                pct[e.stat] += float(e.value) * e.stacks
            elif e.value_type == "multiplier":
                mult[e.stat] += float(e.value) * e.stacks
            else:
                result[e.stat] += float(e.value) * e.stacks

        for stat,v in pct.items():
            # Percent modifiers are additive within the same stat bucket.
            result[stat] *= 1.0 + v
        for stat,v in mult.items():
            result[stat] *= 1.0 + v

        result["crit_rate"] = max(0.0, min(1.0, result.get("crit_rate", .05)))
        result["crit_dmg"] = result.get("crit_dmg", .50)
        result["max_hp"] = result.get("max_hp", result.get("hp", 0))
        result["hp"] = result.get("hp", result["max_hp"])
        result["spd"] = max(1.0, result.get("spd", 100.0))
        return dict(result)
