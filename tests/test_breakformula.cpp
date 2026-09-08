// Break-formula verification against the research notes
// (fribbels/hsr-optimizer, MIT): single-package constants 3767.5533 +
// ElementScaling + (0.5 + T/120), plus the boost terms. All expected
// values below are hand calculations (see comments), not code output.
// Plain asserts, zero deps.
#include "simulation/DamageCalculator.h"

#include <cmath>
#include <cstdio>

namespace dmg = hsr::damage;

static int failures = 0;
#define CHECK(cond) do { \
    if (!(cond)) { \
        ++failures; \
        std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

namespace
{

dmg::BreakDamageConfig baseConfig()
{
    dmg::BreakDamageConfig bc;
    bc.attackerLevel = 80;
    bc.attackerElement = "Fire";
    bc.enemyMaxToughness = 60.0;
    bc.defenseConfig.attackerLevel = 80;
    bc.defenseConfig.enemyBaseDEF = 0.0; // DEF stage = 1.0
    bc.resistanceConfig.explicitBaseRES = 0.2; // neutral 20% -> x0.8
    bc.universalReductionConfig.isEnemyBroken = false; // x0.9
    return bc;
}

} // namespace

int main()
{
    // Hand calc: 3767.5533 x Fire 2.0 = 7535.1066 (baseBreak);
    // toughness 0.5 + 60/120 = 1.0; DEF 1.0; RES 0.8; vuln 1.0;
    // unbroken 0.9. Final = 7535.1066 x 1.0 x 0.8 x 0.9 = 5425.276752.
    {
        dmg::BreakDamageResult r = dmg::calculateBreakDamage(baseConfig());
        CHECK(std::fabs(r.baseBreak - 7535.1066) < 0.01);
        CHECK(std::fabs(r.toughnessMultiplier - 1.0) < 1e-9);
        CHECK(std::fabs(r.finalBreakDamage - 5425.276752) < 0.05);
    }

    // Toughness-term discriminator: T=120 -> 1.5, T=0 -> 0.5 (ratio 3.0).
    // The old /40 divisor would give 3.5 / 0.5 = 7.0.
    {
        dmg::BreakDamageConfig hi = baseConfig();
        hi.enemyMaxToughness = 120.0;
        dmg::BreakDamageConfig lo = baseConfig();
        lo.enemyMaxToughness = 0.0;
        double rHi = dmg::calculateBreakDamage(hi).finalBreakDamage;
        double rLo = dmg::calculateBreakDamage(lo).finalBreakDamage;
        CHECK(std::fabs(rHi / rLo - 3.0) < 1e-9);
    }

    // Boost terms: finalDmgBoost 0.25 -> x1.25 = 6781.59594.
    {
        dmg::BreakDamageConfig bc = baseConfig();
        bc.finalDmgBoost = 0.25;
        dmg::BreakDamageResult r = dmg::calculateBreakDamage(bc);
        CHECK(std::fabs(r.finalBreakDamage - 6781.59594) < 0.05);
    }

    // Super Break hand calc: (3767.5533/10) x 20 = 7535.1066;
    // x0.8 (RES) x1.0 (broken) = 6028.08528.
    {
        double sb = dmg::calculateSuperBreakDamage(20.0, 0.0, 1.0,
                                                   1.0, 0.8, 1.0, 1.0);
        CHECK(std::fabs(sb - 6028.08528) < 0.05);
        // Hit-level boost 0.5 -> x1.5.
        double boosted = dmg::calculateSuperBreakDamage(20.0, 0.0, 1.0,
                                                        1.0, 0.8, 1.0, 1.0,
                                                        0.5, 0.0, 0.0);
        CHECK(std::fabs(boosted - 6028.08528 * 1.5) < 0.05);
    }

    if (failures == 0)
        std::printf("test_breakformula: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
