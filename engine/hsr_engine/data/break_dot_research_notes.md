# Break / DoT / Skill-Table Research — Source Notes

Source: `fribbels/hsr-optimizer` (MIT licensed, github.com/fribbels/hsr-optimizer),
commit `d28928b` (2026-09-07). Pulled directly from the repo via sparse `git clone`,
not from wiki prose — this is the actual TS running in the most widely used
open-source HSR optimizer, so treat it as production-grade, not a secondary summary.
Retain the MIT notice if any of this is copied into your codebase verbatim.

## 1. Break damage — resolved formula (verified, not provisional)

```
BreakDamage =
    weaknessBrokenMulti          // 1.0 if enemy currently Weakness Broken, else 0.9
  × defMulti                     // 100 / ((enemyLevel + 20) * (1 - DEF_PEN) + 100)
  × resMulti                     // 1 - clamp(enemyRES - RES_PEN, -1.00, 0.90)
  × vulnMulti                    // 1 + clamp(vulnerability, 0, 2.50)
  × finalDmgMulti                // 1 + finalDmgBoost
  × dmgBoostMulti                // 1 + hitLevelBoost   <- break only reads HIT-level Boost,
                                  //    NOT action-level or elemental DMG% (matches "Break
                                  //    can't use ATK/CRIT/DMG%" from HoYoWiki)
  × breakBaseMulti
  × beMulti                      // 1 + character.breakEffect
  × trueDmgMulti                 // 1 + trueDmgModifier

breakBaseMulti = 3767.5533 × ElementScaling[element] × (0.5 + enemyMaxToughness / 120) × specialScaling

ElementScaling = {
  Physical: 2.0, Fire: 2.0,
  Ice: 1.0, Lightning: 1.0,
  Wind: 1.5,
  Quantum: 0.5, Imaginary: 0.5,
}
```
`3767.5533` is the level-80 Base Break constant (matches the level-multiplier figure
independently reported by HoYoWiki as "~3767" at level 80 — the repo has it to 4dp).
`enemyMaxToughness` is your `Enemy.toughness` field directly, no unit conversion needed.

Note: wiki sources describe the toughness term as `(Toughness + 20) / 40`, this repo
uses `0.5 + Toughness / 120`. These are NOT the same formula in isolation, but they
don't need to match term-for-term — take the whole constant set (3767.5533 +
ElementScaling + this toughness term) as one internally-consistent package from a
single source rather than mixing terms across sources.

## 2. Super Break — resolved formula

```
SuperBreakDamage =
    weaknessBrokenMulti × defMulti × resMulti × vulnMulti × finalDmgMulti
  × dmgBoostMulti           // hit-level Boost only, same as Break
  × superBreakBaseMulti
  × beMulti
  × superBreakModMulti      // stat: SUPER_BREAK_MODIFIER, buffed by things like Watchmaker/
                             // Harmony MC's Backup Dancer — 0 by default, so SuperBreak
                             // damage is 0 unless something grants this modifier
  × trueDmgMulti

superBreakBaseMulti = (3767.5533 / 10) × effectiveToughness
effectiveToughness  = (1 + breakEfficiencyBoost) × referenceHit.toughnessDmg + referenceHit.fixedToughnessDmg
```
`referenceHit` = the attack that's triggering the super break tick (its toughness-damage
value feeds back in as SuperBreak's scaling input).

## 3. Enemy RES default — cross-check with your Q2 decision

`enemyResistance: 0.2` is hardcoded as the default in this repo's form config. Confirms
the 20%-default / 0%-weakness rule from Q2 is correct as the baseline. This repo has no
enemy/boss database though (it's player-input driven, not encounter-database driven),
so it can't corroborate specific boss RES override values — your `resistances{}` map
still needs its own data source per boss.

## 4. DoT infliction chance — general formula + per-character table

```
effectiveDotChance = min(1, dotBaseChance × (1 + EHR) × (1 - enemyEffectRes + EffectResPen))
```
This is a hit-probability multiplier applied to that hit's damage expectation (used for
EV-style calculation, not a random roll) — i.e. it multiplies expected DoT damage output
by the chance the DoT actually lands. `dotStacks`/`dotSplit` params (not shown above)
handle multi-stack DoTs like Sampo/Guinaifen where refreshing early forfeits some stacks.

Per-character `dotBaseChance` values, extracted from all 103 character files
(see `dot_base_chance.csv`, 15 rows — most of the 103 characters have no kit DoT at all,
so 15/103 is expected, not a parsing miss):

| Character | dotBaseChance |
|---|---|
| Sampo | 0.65 |
| Himeko | 0.50 |
| Asta | 0.80 |
| Jiaoqiu | 1.00 |
| Luka | 1.00 |
| Serval | 1.00 |
| Hook | 1.00 |
| KafkaB1 | 1.00 |
| Hysilens | 1.00 (×5 separate hits, all 1.00) |
| BlackSwanB1 | computed (`dotChance` variable — not a literal, needs manual read of the source file) |
| Guinaifen | computed (`dotChance` variable — same caveat) |

## 5. Character skill/talent scaling tables ("#1[i]%" values)

These live one file per character at
`src/lib/conditionals/character/{pathId}/{CharacterName}.ts`, ~103 files. The data model
is **not** a full per-level table — it's a `(baseValue, eidolonBoostedValue)` pair, because
a build optimizer assumes talents are maxed and only cares whether the relevant Eidolon
(E3 or E5, sometimes E1) is unlocked. Example, `DanHeng.ts`:

```ts
const basicScaling = basic(e, 1.00, 1.10)   // 100% ATK base, 110% ATK at E3
const skillScaling = skill(e, 2.60, 2.86)   // 260% ATK base, 286% ATK at E3
const ultScaling   = ult(e, 4.00, 4.32)     // 400% ATK base, 432% ATK at E5
```
`AbilityEidolon.SKILL_BASIC_3_ULT_TALENT_5` (declared per-character) tells you which pair
(basic/skill vs ult/talent) breaks at E3 vs E5 — this varies by character, it's not fixed.

**Extracted `skill_scaling_raw.csv`** (594 rows, all 103 files, regex-extracted): columns
are `character_file, path_id, ability_kind, variable_name, min_value, eidolon_value,
eidolon_value_2, raw_args, literal`. 593/594 rows are literal numeric constants;
`variable_name` is the TS variable each value was assigned to, which is usually
self-explanatory (`basicScaling`, `ultExtraScaling`, `talentPenBuff`, etc.) but is a
best-effort text label, not a controlled vocabulary — expect to normalize it before it
becomes a schema column in your project.

**One row needs manual resolution**: `Luka.basic` = `0.20 * 3 + 0.80, 0.22 * 3 + 0.88`
→ evaluates to `1.40, 1.54`, kept as the raw expression since it's arithmetic rather
than a bare literal, and I didn't want to silently eval and hide that from you.

**This is a first-pass regex extraction, not a verified import** — spot-check a sample
against the source files (or in-game) before trusting it as ground truth. It's a much
stronger starting point than re-deriving ~600 values from wiki text by hand, but it
hasn't been cross-checked value-by-value.

## 6. Exo-Toughness — confirmed gap, not a research miss

Searched the full `src/lib` and `src/types` tree of this repo (991 TS files) for
`exo`/`ExoToughness`/`multi-layer toughness` in any form: zero matches. Even the most
complete open-source HSR optimizer doesn't model Exo-Toughness as a distinct mechanic.
This isn't a source I failed to find — it's genuinely undocumented in the tooling
ecosystem outside HoYoverse's own client. Recommend it stay config-level/manual, as you
already planned; there's no external reference to promote it beyond that right now.
