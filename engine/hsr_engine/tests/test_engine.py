from hsr_engine import *

def test_damage():
    a = Unit("a","Attacker",80,{"atk":1000,"def":500,"spd":100,"crit_rate":.05,"crit_dmg":.5})
    t = Unit("t","Target",95,{"hp":10000,"def":1000,"spd":100})
    t.states["elemental_res"] = {"fire":0.0}
    state = BattleState({"a":a,"t":t})
    action = Action("skill","a","skill",["t"],1.0,"atk","fire")
    result = RuleEngine().use_action(state, action)
    assert result.final > 0
    assert t.hp < 10000
