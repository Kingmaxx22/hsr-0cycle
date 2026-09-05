from .effects import EffectEngine
from .stats import StatEngine
from .damage import DamageEngine

class RuleEngine:
    def __init__(self):
        self.effects = EffectEngine()
        self.stats = StatEngine()
        self.damage = DamageEngine()

    def refresh_stats(self, state):
        for u in state.units.values():
            u.stats = self.stats.calculate(u, state)
            u.max_hp = u.stats.get("max_hp", u.max_hp)
            u.hp = min(u.hp, u.max_hp)

    def emit(self, event, actor, target, state, **payload):
        state.events.append({
            "time": state.time, "event": event,
            "actor": actor.id, "target": target.id if target else None, **payload
        })
        self.effects.process_trigger(event, actor, target, state)

    def use_action(self, state, action):
        actor = state.units[action.actor_id]
        target = state.units[action.target_ids[0]] if action.target_ids else None

        if state.skill_points < action.sp_cost:
            raise ValueError("Not enough Skill Points")
        state.skill_points -= action.sp_cost

        if actor.energy < action.energy_cost:
            raise ValueError("Not enough Energy")
        actor.energy -= action.energy_cost

        self.emit("before_" + action.kind, actor, target, state, action=action.id)

        result = None
        if action.multiplier and target and action.damage_type:
            result = self.damage.calculate(actor, target, action, state, crit=False)
            target.hp = max(0.0, target.hp - result.final)
            if target.hp <= 0:
                state.defeated.add(target.id)
                self.emit("enemy_defeated", actor, target, state)

        actor.energy = min(actor.max_energy, actor.energy + action.energy_gain)
        self.emit("after_" + action.kind, actor, target, state,
                  damage=result.final if result else 0.0)
        self.refresh_stats(state)
        return result
