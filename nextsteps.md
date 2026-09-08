# HSR 0-Cycle — Next Steps

This roadmap breaks the missing simulator work into small, testable milestones. Keep one milestone per commit.

## Implementation rules

- Preserve working systems; make the smallest reasonable change.
- Inspect code before changing it.
- Keep combat rules data-driven instead of hardcoding every character.
- Add tests with every mechanic.
- Keep deterministic replay possible.
- Every milestone: build -> tests -> review diff -> commit.
- Use explicit `git add <paths>`, not `git add -A`.
- Keep content source/version/hash provenance.
- Do not import unreleased/future game content into the live database.

---

# Phase 0 — Foundation

## 0.1 Test infrastructure
- CMake test target.
- Python test harness where applicable.
- Shared combat fixtures.
- Golden damage-formula tests.
- Deterministic RNG fixture.
- Short end-to-end simulation test.

Commit: `Milestone 0.1: Establish simulator test infrastructure`

## 0.2 Deterministic RNG
- One RNG service for crits, random targeting, bounce, and future RNG.
- Fixed seed reproduces the exact replay.
- Seed stored in simulation config/results.
- Optional deterministic/no-RNG mode.

Commit: `Milestone 0.2: Add deterministic simulation RNG`

## 0.3 Unified actor model
Unify characters, enemies, summons, and memosprites under a combat actor abstraction with stats, resources, AV, actions, statuses, and targeting.

Commit: `Milestone 0.3: Establish unified combat actor model`

## 0.4 Generic effects/status framework
Create reusable effects with source, target, type, value, stacks, duration, trigger, condition, snapshot/dynamic behavior, dispel behavior, and manual enabled state.

Commit: `Milestone 0.4: Add generic combat effect framework`

---

# Phase 1 — Full Combat

## 1.1 Real Action Value timeline
Implement mutable action gauge and exact:
- Speed changes
- Advance Forward
- Action Delay
- Action Advance
- simultaneous actions
- immediate Ultimate insertion
- event ordering

Tests for advance, delay, speed changes, break delay, and ties.

Commit: `Milestone 1.1: Implement mutable Action Value timeline`

## 1.2 Full multi-target/splash resolution
Implement:
- Single Target
- Blast
- AoE
- Bounce
- splash/multi-target attacks

Resolve each target independently for damage, weakness, toughness, debuffs, death, and overkill. Respect slot order and dead-target skipping.

Commit: `Milestone 1.2: Implement full multi-target and splash resolution`

## 1.3 Combat event pipeline
Events:
- skill start
- hit
- damage
- heal
- buff/debuff
- toughness damage
- break
- FUA
- DoT tick
- energy/SP changes
- advance/delay
- death

Commit: `Milestone 1.3: Add combat event pipeline`

## 1.4 SP system
Implement skill costs, Basic generation, conditional/enhanced skills, SP modifiers, and exact team SP timeline.

Commit: `Milestone 1.4: Implement combat Skill Point economy`

## 1.5 Energy system
Implement max/current energy, action-based gain, enemy-hit gain, Break/FUA/DoT interactions, ERR, technique/start energy, overflow, drains/locks, and manual Ultimate insertion.

Commit: `Milestone 1.5: Implement combat energy system`

## 1.6 Break + Super Break
Implement toughness, weakness matching, toughness damage, Break detection, element Break effects, Break damage, Break Effect, action delay, broken/recovery state, repeated breaking, and Super Break.

Commit: `Milestone 1.6: Implement weakness break and Super Break`

## 1.7 DoT engine
Implement Burn, Shock, Wind Shear, Bleed, character DoTs, stacks, duration/refresh, snapshot vs dynamic stats, tick timing, immediate DoT triggers, and source tracking.

Commit: `Milestone 1.7: Implement damage over time engine`

## 1.8 Follow-up attacks
Generic condition/event-based FUA triggers, target selection, multi-hit FUA, FUA resource interactions, and recursion guards.

Commit: `Milestone 1.8: Implement generic follow-up attack system`

## 1.9 Summons and memosprites
Treat independent summons/memosprites as actors where required, including actions, AV, stats, targeting, resources, and owner linkage.

Commit: `Milestone 1.9: Implement summons and memosprite actors`

---

# Phase 2 — Character Completeness

## 2.1 Data-driven skills
Represent Basic/Skill/Talent/Ultimate/Technique/FUA/Enhanced variants with scaling, element, target pattern, hits, toughness damage, energy/SP behavior, effects, conditions, and triggers.

Commit: `Milestone 2.1: Convert character actions to data-driven definitions`

## 2.2 Traces
Store major traces, minor stat traces, levels, unlocks, effects, and manual enable/disable state.

Commit: `Milestone 2.2: Add data-driven character Traces`

## 2.3 Eidolons E0-E6
Every character supports E0-E6. Each Eidolon is a separate data/effect entry. UI exposes E-level/manual toggles. E3/E5 level effects are represented separately.

Tests compare E0 against each enabled Eidolon effect.

Commit: `Milestone 2.3: Add E0-E6 Eidolon system`

## 2.4 Light Cone passives
Store identity, path, rarity, base stats, S1-S5, passive text, parsed/manual effects, conditions, and manual toggle.

Commit: `Milestone 2.4: Add data-driven Light Cone passives`

## 2.5 Manual combat toggles
UI toggles for Eidolons, Traces, Light Cone passives, conditional buffs, Technique effects, and scenario assumptions. Include toggle state in replay/log output.

Commit: `Milestone 2.5: Add manual combat effect toggles`

---

# Phase 3 — Team Simulation

## 3.1 Buff/debuff resolution
Support ATK/HP/DEF/SPD, DMG%, RES reduction/PEN, DEF reduction/ignore, vulnerability, crit, Break Effect, ERR, Advance/Delay, taunt, effect hit/resistance, and dispel.

Commit: `Milestone 3.1: Implement generic buff and debuff resolution`

## 3.2 Snapshot semantics
Explicitly support snapshot-at-cast, snapshot-per-hit, dynamic-at-damage, and dynamic-at-tick behavior.

Commit: `Milestone 3.2: Implement stat snapshot semantics`

## 3.3 Enemy AI
Enemy actions, target selection, taunt, buffs/debuffs, enemy FUA, summons, delayed/forced actions, phases, thresholds, and death/spawn events.

Commit: `Milestone 3.3: Implement enemy action and targeting system`

## 3.4 Encounter scripting
Waves, spawns, phase transitions, boss scripts, turn/round triggers, HP thresholds, victory/failure conditions, stage buffs, and restrictions.

Commit: `Milestone 3.4: Implement encounter scripting and waves`

---

# Phase 4 — 0-Cycle Optimization

## 4.1 Rotation policies
Explicit deterministic policies for skill/basic/Ultimate timing, target selection, FUA/DoT behavior, and conditional actions.

Commit: `Milestone 4.1: Add deterministic rotation policies`

## 4.2 0-cycle metrics
Track cycle count, elapsed AV, actions, damage over time, enemy HP, Break state, SP, Energy, deaths, and wave completion.

Commit: `Milestone 4.2: Add 0-cycle evaluation metrics`

## 4.3 Search/optimization
Search speed tuning, action ordering, target selection, Ultimate timing, Eidolons, Light Cones, relic stats, and rotations. Preserve deterministic replay for winning candidates.

Commit: `Milestone 4.3: Add rotation and build optimization search`

## 4.4 Monte Carlo / EV mode
Mean, median, percentiles, min/max, standard deviation, and probability of hitting a 0-cycle target.

Commit: `Milestone 4.4: Add Monte Carlo simulation mode`

---

# Phase 5 — Content Accuracy + Data Pipeline

This phase should be a **versioned data pipeline**, not a giant hardcoded C++ database.

## Recommended research/source stack

### Characters / Traces / Eidolons / Light Cones
**Primary: Mar-7th/StarRailRes**

Use its structured data for:
- character IDs/names
- path/element/rarity
- base stats/promotions
- skills and skill levels
- Eidolon/rank definitions
- skill trees/Traces
- Light Cone stats
- Light Cone ranks/S1-S5
- icons/assets where appropriate

Prefer structured repository/API data over HTML scraping.

### Enemies + numeric stage data
**Primary candidate: Hakush.in**

Use structured data for:
- enemy IDs
- HP/ATK/DEF/SPD
- toughness/weakness information where available
- stage IDs
- wave composition
- enemy levels
- stage modifiers

Do not assume it contains every boss script/mechanic.

### Secondary cross-check
**StarRailStation-derived datasets**

Use for:
- missing/new characters
- Light Cone cross-checking
- ID/name mismatches
- text changes
- missing assets

Treat as validation/fallback, not the sole source of truth.

### Raw game-data fallback
**Dimbreath/StarRailData**

Use when a field is missing and you need to inspect upstream game resources. Prefer consuming normalized StarRailRes data for the actual project pipeline.

### Mechanics verification
**Honkai: Star Rail Wiki/Fandom**

Use for human-readable verification of:
- enemy skills
- boss mechanics
- special triggers
- weaknesses/resistances
- unusual encounter behavior

Do not scrape wiki prose directly into simulator logic. Convert verified mechanics into structured effects and store the source/version.

---

## 5.1 Data schema + provenance

Suggested structure:

```text
data/
  raw/
    starrailres/
    hakushin/
    starrailstation/
    wiki/
    stages/
  normalized/
    characters/
    light_cones/
    enemies/
    encounters/
    stages/
  manifests/
    sources.json
    versions.json
```

Every normalized record should include:

```json
{
  "id": "...",
  "name": "...",
  "source": "...",
  "source_version": "...",
  "source_url": "...",
  "retrieved_at": "...",
  "content_hash": "..."
}
```

Prefer reproducible download/parser scripts over committing enormous raw datasets.

Commit: `Milestone 5.1: Establish versioned content data schemas`

## 5.2 Character importer
Downloader/parser for StarRailRes:
- characters
- skills
- Traces
- Eidolons
- promotions/stats
- validation and reference resolution

Commit: `Milestone 5.2: Add automated character data import`

## 5.3 Light Cone importer
Import base stats, promotions, path/rarity, S1-S5 ranks, passive text, IDs/names, and validation.

Keep passive text even before automatic effect parsing is complete.

Commit: `Milestone 5.3: Add automated Light Cone data import`

## 5.4 Enemy importer
Combine Hakushin structured data with project schema and wiki mechanic verification:
- stats
- level scaling
- weaknesses/RES
- toughness
- enemy skills
- status resistance
- targeting
- summons
- phases

Commit: `Milestone 5.4: Add automated enemy data import`

## 5.5 Memory of Chaos importer
Import current + historical rotations:
- season/rotation
- stage
- waves
- enemy IDs/levels
- stage buffs
- special rules
- boss phases
- spawn conditions

Keep historical rotations for reproducible old 0-cycle results.

Commit: `Milestone 5.5: Add Memory of Chaos encounter data`

## 5.6 Pure Fiction importer
Support:
- waves
- enemy lineups
- stage effects
- scoring/kill rules
- spawn pacing
- stage-specific rules

Commit: `Milestone 5.6: Add Pure Fiction encounter data`

## 5.7 Apocalyptic Shadow importer
Support:
- bosses
- phases
- weakness changes
- boss mechanics
- stage effects
- wave/spawn behavior
- victory conditions

Commit: `Milestone 5.7: Add Apocalyptic Shadow encounter data`

## 5.8 Generic boss phase/script format
Example:

```yaml
phase:
  id: phase_2
  trigger:
    type: hp_threshold
    value: 0.50
  actions:
    - spawn: enemy_id
    - apply_effect: effect_id
    - change_weaknesses: [fire, ice]
    - advance_action: 100
```

Bosses should execute generic events rather than bespoke C++ scripts.

Commit: `Milestone 5.8: Add data-driven boss phase scripting`

## 5.9 Reproducible content update command
Create something like:

```text
python tools/update_hsr_data.py
```

It should:
1. Download upstream datasets.
2. Record source revision/date.
3. Filter unreleased/future content.
4. Parse and normalize.
5. Validate references.
6. Generate manifests.
7. Report added/removed/changed records.
8. Run content tests.

Commit: `Milestone 5.9: Add reproducible HSR content update pipeline`

## 5.10 Content regression tests
Test:
- no duplicate IDs
- no dangling skill/Trace/Eidolon refs
- every encounter enemy exists
- every wave resolves
- every boss phase references valid events
- no future/unreleased content leaks into the live dataset
- source manifests exist
- imported counts change only when expected

Commit: `Milestone 5.10: Add content database regression tests`

---

# Phase 6 — Accuracy Validation

## 6.1 Formula golden tests
Known-value tests for every damage layer.

## 6.2 Timeline golden tests
Known SPD/AV/Advance/Delay action-order fixtures.

## 6.3 Character kit golden tests
Representative Basic/Skill/Talent/Ultimate/FUA/Technique/Trace/Eidolon/Light Cone cases.

## 6.4 Encounter replay tests
Deterministic replay fixtures for selected MoC/PF/AS stages.

Commit: `Milestone 6.x: Add combat and encounter accuracy regression suite`

---

# Scraping policy

Prefer:

1. Structured game-data repository/API.
2. Versioned JSON endpoint.
3. Public static API.
4. Wiki/API extraction for mechanics missing from structured data.
5. HTML scraping only as a last resort.

Practical stack:

```text
Characters / Traces / Eidolons / Light Cones -> StarRailRes
Enemy numeric data / stage composition       -> Hakush.in
Mechanics / special boss behavior             -> Wiki/Fandom
Secondary cross-check                         -> StarRailStation datasets
Raw resource investigation                     -> StarRailData
```

For every source record:
- game version
- upstream revision
- retrieval timestamp
- content hash
- parser version
- imported/changed/rejected counts

Check upstream licenses before redistributing copied data. Prefer a downloader/parser plus normalized project data rather than copying entire upstream repositories.

---

# Recommended order

```text
0.1 Tests
0.2 RNG
0.3 Actors
0.4 Effects

1.1 AV
1.2 Multi-target
1.3 Events
1.4 SP
1.5 Energy
1.6 Break/Super Break
1.7 DoT
1.8 FUA
1.9 Summons/Memosprites

2.1 Skills
2.2 Traces
2.3 Eidolons
2.4 Light Cones
2.5 Manual toggles

3.1 Buff/debuffs
3.2 Snapshots
3.3 Enemy AI
3.4 Encounter scripts

4.1 Rotations
4.2 0-cycle metrics
4.3 Optimizer
4.4 Monte Carlo

5.1 Data schema
5.2 Characters
5.3 Light Cones
5.4 Enemies
5.5 MoC
5.6 PF
5.7 AS
5.8 Boss scripts
5.9 Update pipeline
5.10 Content tests

6.x Accuracy validation
```

# Definition of done

The simulator is ready for serious 0-cycle work when:

- A full 4-character team can run deterministic multi-target encounters.
- Single/Blast/AoE/Bounce resolve correctly.
- SP and Energy support real rotations.
- AV/Advance/Delay is event-driven.
- Break/Super Break and DoT are real systems.
- FUA and summons/memosprites can act.
- Traces, Eidolons, and Light Cone passives exist as data and are manually toggleable.
- Enemies can act and bosses can execute scripted phases.
- MoC/PF/AS encounters can be imported rather than manually hardcoded.
- Character/Light Cone/enemy data can be refreshed from upstream sources.
- Every imported record has provenance/version metadata.
- Tests cover formulas, mechanics, content references, and deterministic replays.
- A simulation result is reproducible from configuration + content version + seed.
- Every milestone is independently buildable/testable and committed separately.
