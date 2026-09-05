from hsr_engine import *

engine = RuleEngine()
attacker = Unit("a","Example DPS",80,{"atk":1000,"def":500,"spd":100,"crit_rate":.05,"crit_dmg":.50})
target = Unit("e","Example Enemy",95,{"hp":20000,"def":1150,"spd":132})
target.states["elemental_res"]={"lightning":.20}
target.states["weaknesses"]={"lightning"}
state=BattleState({"a":attacker,"e":target})
attacker.effects.append(Effect("relic_crit","stat_modifier","Example Relic",stat="crit_rate",value=.08,value_type="percent"))
engine.refresh_stats(state)
action=Action("ult","a","ultimate",["e"],2.0,"atk","lightning")
print(engine.use_action(state,action))
print("Enemy HP:",target.hp)
