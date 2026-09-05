from dataclasses import replace
from .model import Effect, Unit, BattleState, Condition

class ConditionEngine:
    def check(self, c, actor, target, state):
        if c is None: return True
        k, a = c.kind, c.args
        if k == "always": return True
        if k == "state":
            u = actor if a.get("subject","actor") == "actor" else target
            return bool(u and u.states.get(a["state"]))
        if k == "stat_gte":
            u = actor if a.get("subject","actor") == "actor" else target
            return bool(u and u.stats.get(a["stat"], 0) >= float(a["value"]))
        if k == "hp_below":
            u = actor if a.get("subject","actor") == "actor" else target
            return bool(u and u.hp <= u.max_hp * float(a["ratio"]))
        if k == "hp_above":
            u = actor if a.get("subject","actor") == "actor" else target
            return bool(u and u.hp >= u.max_hp * float(a["ratio"]))
        if k == "target_weakness":
            return bool(target and a["element"] in target.states.get("weaknesses", set()))
        if k == "enemy_debuff_count_gte":
            return bool(target and len(target.states.get("debuffs", {})) >= int(a["value"]))
        if k == "tag":
            u = actor if a.get("subject","actor") == "actor" else target
            return bool(u and a["tag"] in u.tags)
        return False

class EffectEngine:
    def __init__(self):
        self.conditions = ConditionEngine()

    def active(self, effect, actor, target, state):
        return self.conditions.check(effect.condition, actor, target, state)

    def collect(self, actor, target, state):
        return [e for e in actor.effects if self.active(e, actor, target, state)]

    def add(self, unit, effect):
        unit.effects.append(effect)

    def process_trigger(self, event, actor, target, state):
        for e in list(actor.effects):
            if e.trigger != event: continue
            if self.active(e, actor, target, state):
                if e.kind == "gain_energy":
                    actor.energy = min(actor.max_energy, actor.energy + float(e.value))
                elif e.kind == "gain_sp":
                    state.skill_points = min(state.max_skill_points,
                                             state.skill_points + int(e.value))
                elif e.kind == "state":
                    actor.states[str(e.metadata["state"])] = e.value
                elif e.kind == "add_effect" and target:
                    self.add(target, replace(e, trigger=None))
