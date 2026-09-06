"""
Tests for the HSR 0-Cycle Simulation Engine
"""

import unittest
from hsr_engine import (
    AVCalculator,
    ActionAdvancer,
    SimulationEngine,
    SimAction,
    ActionType,
    calculate_speed_breakpoint,
    get_speed_table,
    simulate_0cycle
)


class TestAVCalculator(unittest.TestCase):
    """Test Action Value calculations"""
    
    def test_basic_av_calculation(self):
        """Test AV = 10000 / SPD formula"""
        self.assertAlmostEqual(AVCalculator.calculate_av(100), 100.0)
        self.assertAlmostEqual(AVCalculator.calculate_av(134), 74.62686567164179, places=5)
        self.assertAlmostEqual(AVCalculator.calculate_av(200), 50.0)
    
    def test_zero_speed(self):
        """Test handling of zero/negative speed"""
        self.assertEqual(AVCalculator.calculate_av(0), float('inf'))
        self.assertEqual(AVCalculator.calculate_av(-10), float('inf'))
    
    def test_reverse_calculation(self):
        """Test calculating SPD from AV"""
        self.assertAlmostEqual(AVCalculator.calculate_spd_for_av(100.0), 100.0)
        self.assertAlmostEqual(AVCalculator.calculate_spd_for_av(50.0), 200.0)
    
    def test_breakpoint_134_spd(self):
        """Test key breakpoint: 134 SPD = 2 actions in 150 AV"""
        bp = calculate_speed_breakpoint(134)
        self.assertEqual(bp.actions_in_150_av, 2)
        self.assertEqual(bp.actions_in_cycle, 1)
        self.assertLess(bp.first_action_av, 75.0)
    
    def test_breakpoint_200_spd(self):
        """Test key breakpoint: 200 SPD = 3 actions in 150 AV"""
        bp = calculate_speed_breakpoint(200)
        self.assertEqual(bp.actions_in_150_av, 3)
        self.assertEqual(bp.actions_in_cycle, 2)
        self.assertAlmostEqual(bp.first_action_av, 50.0)
    
    def test_speed_table_exists(self):
        """Test that speed table has entries"""
        table = get_speed_table()
        self.assertGreater(len(table), 0)
        # Should have common breakpoints
        spd_values = [bp.spd_required for bp in table]
        self.assertIn(134, spd_values)
        self.assertIn(200, spd_values)


class TestActionAdvancer(unittest.TestCase):
    """Test action advance mechanics"""
    
    def test_flat_advance(self):
        """Test -24 AV advance (Bronya skill)"""
        new_av = ActionAdvancer.apply_flat_advance(74.6, 24.0)
        self.assertAlmostEqual(new_av, 50.6)
    
    def test_flat_advance_minimum(self):
        """Test advance doesn't go below minimum"""
        new_av = ActionAdvancer.apply_flat_advance(20.0, 24.0, min_av=0.0)
        self.assertEqual(new_av, 0.0)
    
    def test_percent_advance(self):
        """Test percentage advance"""
        full_cycle_av = 100.0  # 100 SPD
        new_av = ActionAdvancer.apply_percent_advance(75.0, 0.25, full_cycle_av)
        self.assertAlmostEqual(new_av, 50.0)  # 75 - (100 * 0.25)


class TestSimulationEngine(unittest.TestCase):
    """Test simulation engine functionality"""
    
    def test_setup_simulation(self):
        """Test simulation state initialization"""
        engine = SimulationEngine()
        units = {'dps': {'name': 'Test', 'spd': 134, 'hp': 1000}}
        enemies = {'boss': {'name': 'Boss', 'spd': 100, 'hp': 50000}}
        
        state = engine.setup_simulation(units, enemies)
        
        self.assertIn('dps', state.units)
        self.assertIn('boss', state.units)
        self.assertEqual(state.skill_points, 3)
        self.assertEqual(state.av_limit, 150.0)
    
    def test_auto_simulate_basic(self):
        """Test basic auto-simulation runs without errors"""
        units = {
            'dps': {'name': 'DPS', 'spd': 134, 'hp': 1000},
            'support': {'name': 'Support', 'spd': 120, 'hp': 800}
        }
        enemies = {
            'boss': {'name': 'Boss', 'spd': 100, 'hp': 10000, 'first_turn_delay': 100.0}
        }
        
        result = simulate_0cycle(units, enemies, auto=True)
        
        self.assertGreater(len(result.actions_taken), 0)
        self.assertGreater(result.total_damage, 0)
        self.assertIsNotNone(result.speed_breakpoints)
    
    def test_custom_rotation(self):
        """Test custom action queue execution"""
        engine = SimulationEngine()
        
        units = {'seele': {'name': 'Seele', 'spd': 142, 'hp': 1000}}
        enemies = {'mob': {'name': 'Mob', 'spd': 100, 'hp': 5000}}
        
        state = engine.setup_simulation(units, enemies, initial_sp=3)
        
        actions = [
            SimAction(
                av=70.4,
                actor_id='seele',
                action_type=ActionType.SKILL,
                target_ids=['mob'],
                multiplier=2.0,
                sp_gain=-1
            )
        ]
        
        result = engine.simulate_rotation(state, actions)
        
        self.assertEqual(len(result.actions_taken), 1)
        self.assertEqual(result.actions_taken[0].actor_id, 'seele')
    
    def test_skill_point_tracking(self):
        """Test SP is tracked correctly through rotation"""
        engine = SimulationEngine()
        
        units = {'char': {'name': 'Char', 'spd': 134, 'hp': 1000}}
        enemies = {'mob': {'name': 'Mob', 'spd': 100, 'hp': 5000}}
        
        state = engine.setup_simulation(units, enemies, initial_sp=3)
        
        # Skill (-1 SP) then Basic (+1 SP)
        actions = [
            SimAction(av=74.6, actor_id='char', action_type=ActionType.SKILL,
                     target_ids=['mob'], sp_gain=-1),
            SimAction(av=149.3, actor_id='char', action_type=ActionType.BASIC,
                     target_ids=['mob'], sp_gain=1)
        ]
        
        result = engine.simulate_rotation(state, actions)
        
        # Check SP history
        self.assertEqual(len(result.skill_points_history), 3)  # Initial + 2 actions
        self.assertEqual(result.skill_points_history[-1][1], 3)  # Back to 3 SP
    
    def test_enemy_defeat_detection(self):
        """Test enemy defeat is properly detected"""
        engine = SimulationEngine()
        
        units = {'dps': {'name': 'DPS', 'spd': 134, 'hp': 1000}}
        enemies = {'weak_mob': {'name': 'Weak Mob', 'spd': 100, 'hp': 100}}
        
        state = engine.setup_simulation(units, enemies)
        
        # One strong skill should kill
        actions = [
            SimAction(av=74.6, actor_id='dps', action_type=ActionType.SKILL,
                     target_ids=['weak_mob'], multiplier=5.0, sp_gain=-1)
        ]
        
        result = engine.simulate_rotation(state, actions)
        
        self.assertTrue(result.success)
        self.assertIsNotNone(result.enemy_defeated_at_av)
        self.assertLessEqual(result.enemy_defeated_at_av, 150.0)
    
    def test_zero_cycle_validation(self):
        """Test is_zero_cycle property"""
        units = {'dps': {'name': 'DPS', 'spd': 134, 'hp': 1000}}
        
        # Enemy with low HP (should die in 0-cycle)
        enemies_low = {'mob': {'name': 'Mob', 'spd': 100, 'hp': 100}}
        result_low = simulate_0cycle(units, enemies_low, auto=True)
        
        # Enemy with high HP (should not die in 0-cycle)
        enemies_high = {'boss': {'name': 'Boss', 'spd': 100, 'hp': 500000}}
        result_high = simulate_0cycle(units, enemies_high, auto=True)
        
        # Low HP enemy should be 0-cycle clear
        self.assertTrue(result_low.is_zero_cycle or result_low.success)
        
        # High HP enemy might not be 0-cycle
        # (depends on damage, but at least it should complete)
        self.assertGreater(len(result_high.actions_taken), 0)


class TestIntegration(unittest.TestCase):
    """Test integration scenarios"""
    
    def test_seele_bronya_combo(self):
        """Test Seele + Bronya advance mechanic"""
        units = {
            'seele': {'name': 'Seele', 'spd': 142, 'hp': 1000},
            'bronya': {'name': 'Bronya', 'spd': 134, 'hp': 800}
        }
        enemies = {
            'cocolia': {'name': 'Cocolia', 'spd': 100, 'hp': 80000, 
                       'first_turn_delay': 100.0}
        }
        
        # Custom rotation with advance
        actions = [
            SimAction(av=70.4, actor_id='seele', action_type=ActionType.SKILL,
                     target_ids=['cocolia'], multiplier=2.0, sp_gain=-1),
            SimAction(av=74.6, actor_id='bronya', action_type=ActionType.SKILL,
                     target_ids=['seele'], multiplier=1.0, sp_gain=-1,
                     advance_target='seele', advance_value=24.0),
            # Seele acts again immediately due to advance
            SimAction(av=74.6, actor_id='seele', action_type=ActionType.SKILL,
                     target_ids=['cocolia'], multiplier=2.0, sp_gain=-1),
        ]
        
        engine = SimulationEngine()
        state = engine.setup_simulation(units, enemies, initial_sp=3)
        result = engine.simulate_rotation(state, actions)
        
        # Should have executed all 3 actions
        self.assertEqual(len(result.actions_taken), 3)
        
        # Check timeline order
        avs = [r.av for r in result.actions_taken]
        self.assertEqual(avs, [70.4, 74.6, 74.6])


if __name__ == '__main__':
    unittest.main()
