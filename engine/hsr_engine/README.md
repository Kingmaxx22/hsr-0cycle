# HSR Unified Rules Engine

Foundation for the 0-Cycle simulator.

### One rules vocabulary

Relics, planar ornaments, light cones, character traces, eidolons, skills,
ultimates, follow-ups, DoTs, buffs/debuffs and enemy mechanics all use the
same concepts:

`Effect -> Condition -> Trigger -> Action -> Event -> State`

The simulator should be the only source of truth for combat calculations.

### Current implementation

- Generic `Effect`, `Condition`, `Action`, `Unit`, `BattleState`
- Event/trigger dispatcher
- Final-stat calculation
- Damage pipeline with defense/resistance/vulnerability stages
- Conservative compiler for obvious direct-stat sentences
- Converted copies of the supplied character, relic, light-cone and monster data
- Original scraped descriptions retained so unsupported mechanics are not guessed

### Important

This is the **engine foundation**, not a claim that every HSR mechanic is
already encoded. Complex text such as stacking rules, special gauges, summons,
Break/Super Break, multi-hit damage, action advance/delay, DoT timing and
character-specific mechanics need structured rules added to the same engine.

Run the smoke test with:

`python -m pytest tests`


## Newly integrated character rules

The uploaded `character_skills(1).csv` is now represented by
`data/character_skills_rules.json` (537 records).

The uploaded `character_major_traces(1).csv` is now represented by
`data/character_major_traces_rules.json` (264 records).

Both retain the original row data under `raw` and source descriptions under
`raw_text`. Only mechanics supported by the conservative compiler are turned
into executable effect objects. Unsupported mechanics remain source text for
the next rule-compiler pass.
