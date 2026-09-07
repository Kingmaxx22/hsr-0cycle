# HSR 0-Cycle — OpenCode Agent Instructions

## 1. Project Overview

This repository is an Honkai: Star Rail 0-Cycle damage calculator and simulation project.

The project contains:
- A C++ damage calculation system
- A simulation engine
- Character/enemy/game data
- A Raylib-based UI
- Enemy selection and encounter configuration
- Action Value (AV) and speed calculations
- Damage formula calculations
- Future optimization functionality

Accuracy and maintainability are more important than making large amounts of code quickly.

---

## 2. General Agent Behavior

Make the smallest change necessary to complete the requested task.

Do NOT:
- Rewrite entire files unnecessarily
- Refactor unrelated systems
- Rename large numbers of variables without a reason
- Change architecture without a clear reason
- Replace existing systems with simplified placeholders
- Delete existing functionality because it is inconvenient
- Modify unrelated UI screens
- Modify unrelated simulation logic

Before changing code:
1. Inspect the relevant files.
2. Understand the existing implementation.
3. Identify how the requested change fits the current architecture.
4. Make the smallest reasonable modification.
5. Build the project.
6. Fix compilation errors.
7. Test the affected functionality when possible.

---

## 3. Repository Structure

Important directories:

```text
src/
    Core application/source code

src/simulation/
    Simulation engine
    Damage calculator
    Engine bridge
    Simulation state and combat logic

engine/
    Supporting engine/data implementation

engine/hsr_engine/
    Python/supporting simulation components

engine/hsr_engine/data/
    Character/enemy/game data

ui/
    UI project

ui/src/
    UI implementation

ui/src/screens/
    Individual UI screens

ui/assets/
    Images and visual assets

images/
    Project images

raylib/
    Vendored Raylib dependency

build/
    Generated CMake/build output
```

---

## 4. Directory Priority

When investigating a normal programming task, prioritize:

```text
src/
src/simulation/
engine/
engine/hsr_engine/
engine/hsr_engine/data/
ui/src/
```

Do NOT spend time scanning these unless the task specifically requires them:

```text
build/
build_backup/
ui/assets/
images/
raylib/
**/__pycache__/
```

---

## 5. Build Directory

`build/` contains generated files.

NEVER manually edit files inside `build/`.

If a generated file needs modification, find and modify its source/configuration instead.

The build directory may be deleted and regenerated at any time.

Do not commit generated build output.

---

## 6. Vendored Raylib

`raylib/` is third-party/vendor code.

Do NOT:
- Modify Raylib source
- Refactor Raylib
- Search the entire Raylib source tree unnecessarily
- Make changes inside Raylib to solve project-level problems

Only modify Raylib if explicitly requested or genuinely required.

Prefer fixing our own code around the library.

---

## 7. Assets

Large visual assets are not normally relevant to code tasks.

Normally avoid:

```text
ui/assets/
images/
```

Only inspect assets when the task explicitly involves:
- UI artwork
- Icons
- Character images
- Enemy images
- Asset loading
- Missing/corrupted assets
- Asset paths
- Rendering problems

---

## 8. Do Not Use Placeholder Mechanics

The simulator is intended to model actual HSR combat mechanics.

Do NOT introduce fake or placeholder combat values into production logic.

Do not replace real calculations with values such as:

```cpp
damage = 1000;
```

or:

```cpp
damage = 2500;
```

unless the user explicitly asks for a temporary mock/test.

---

## 9. Damage Calculation

Keep damage calculations modular.

Relevant concepts include:
- Base damage
- ATK/HP/DEF scaling
- DMG%
- Crit DMG
- DEF multiplier
- DEF reduction/shred
- RES
- RES PEN
- Vulnerability
- Special vulnerability
- Damage taken modifiers
- Break-related modifiers
- Character-specific modifiers

When changing a damage formula:
1. Identify the existing formula.
2. Determine which multiplier is being changed.
3. Avoid modifying unrelated multipliers.
4. Preserve existing behavior where the mechanic does not apply.
5. Add tests where practical.

---

## 10. Formula Accuracy

Do not silently simplify game formulas.

If a formula has multiple stages, preserve the stages.

Prefer identifiable stages such as:

```text
Base
→ DMG%
→ Crit
→ DEF
→ RES
→ Vulnerability
→ Final modifiers
```

over an opaque single expression.

Conditional modifiers should remain identifiable when practical.

---

## 11. RES and RES PEN

RES and RES PEN are separate concepts.

Do not arbitrarily clamp RES PEN to 100% if the simulation requires values above 100%.

When implementing RES PEN:
- Preserve the distinction between base RES and effective RES.
- Handle negative effective RES correctly.
- Do not invent additional caps.
- Follow the project's documented damage formula.

---

## 12. Vulnerability

Keep standard vulnerability and special enemy vulnerability conceptually separate.

Do not merge special enemy modifiers into normal vulnerability if that changes their intended behavior.

---

## 13. DEF Reduction / DEF Shred

Be careful with terminology:
- DEF reduction/shred reduces enemy DEF.
- DEF multiplier is calculated from resulting effective DEF.
- Flat DEF reduction must actually reduce DEF.

When modifying DEF calculations, verify:
1. Percent DEF reduction
2. Flat DEF reduction
3. DEF shred caps
4. Enemy level
5. Effective DEF
6. Final DEF multiplier

Do not change the semantics of an existing field without checking all call sites.

---

## 14. Effect Hit Rate

Effect Hit Rate calculations must distinguish:
- Base chance
- Effect RES
- EHR
- Final chance
- Final chance cap
- Multi-hit proc behavior

Do not assume EHR has a 100% cap unless the mechanic requires it.

When implementing multi-hit effects, preserve the probability behavior across multiple hits.

---

## 15. Speed

Speed calculations must distinguish:
- Base Speed
- Percentage Speed
- Flat Speed

The normal relationship is:

```text
Total Speed =
    Base Speed
    + Base Speed × Percentage Speed
    + Flat Speed
```

Percentage Speed scales base speed.

Do not apply percentage Speed to already-added flat Speed.

If a mechanic changes effective base speed, represent that explicitly.

---

## 16. Action Value

AV is a core part of the simulator.

Preserve the relationship:

```text
Speed
↓
Action Value requirement
↓
Action progress
↓
Action Advance / Delay
↓
Next action
```

When modifying AV calculations, check all related mechanics.

---

## 17. Action Advance

Action Advance should modify remaining action requirements rather than simply adding/subtracting a hardcoded AV value.

Be careful about:
- Current action progress
- Remaining AV
- Percentage advance
- 100% caps
- Special delayed-action cases

Do not implement Action Advance as:

```cpp
av -= 10;
```

unless the specific mechanic is a fixed AV amount.

---

## 18. Mid-Turn Speed Changes

When speed changes during an action, do not recalculate the entire action from zero.

Account for action progress already spent.

Conceptually:

```text
Old Speed
↓
Old raw AV
↓
AV already spent
↓
Action progress
↓
Apply advance if applicable
↓
New Speed
↓
Remaining AV using new Speed
```

Test:
- Speed increases
- Speed decreases
- Action Advance
- Mid-turn speed changes

---

## 19. Enemy System

The enemy selection structure uses exactly five enemy slots.

Each slot may contain zero or more enemy IDs.

Example:

```text
Slot 1:
    Enemy A
    Enemy B
    Enemy C

Slot 2:
    Enemy D
    Enemy E

Slot 3:
    Enemy F
    Enemy G

Slot 4:
    empty

Slot 5:
    Enemy H
    Enemy I
```

A slot represents a position/encounter location, not necessarily a single enemy instance.

Do NOT redesign this into five slots with only one enemy ID each.

---

## 20. Enemy Spawning

The simulation must be capable of representing enemies that spawn during combat.

Do not assume the initial enemy list is immutable.

When implementing spawning:
- Preserve slot identity.
- Allow multiple enemy IDs in a slot.
- Track active/inactive enemies appropriately.
- Avoid corrupting target selection when an enemy spawns.
- Do not assume enemy count remains constant.

---

## 21. Character Selection

Character selection should remain flexible.

Do not create a hardcoded character/stat database unless explicitly requested.

Prefer data-driven behavior over assumptions such as:

```cpp
if (character == "Acheron")
```

when a data-driven solution is possible.

Character-specific mechanics should be isolated and extensible.

---

## 22. Character Stats

Avoid duplicate sources of truth.

If the UI/loadout system already knows:

```text
Base ATK
Flat ATK
ATK%
Final ATK
```

the simulation should receive the appropriate calculated value rather than asking the user to manually enter the combined result again.

---

## 23. Relics and Loadouts

When loadout values are available:

```text
Relic stats
+
Character stats
+
Other modifiers
↓
Calculated totals
↓
Simulation input
```

Do not force the user to enter the same combined stat in multiple places.

Determine whether each new value is:
- Raw input
- Derived value
- Simulation-only value
- UI display value

before adding another field.

---

## 24. Simulation Architecture

Keep responsibilities separate where practical:

```text
UI
↓
Configuration / Input Model
↓
Simulation Engine
↓
Combat State
↓
Damage Calculator
↓
Results / Timeline
```

The UI should not contain core combat simulation.

Avoid putting complex damage calculations directly inside `ui/src/screens/` unless they are genuinely UI-specific.

---

## 25. Simulation State

Represent combat state explicitly.

Relevant state may include:
- Current AV
- Current time
- Current cycle
- Character state
- Enemy state
- HP
- Buffs
- Debuffs
- Action progress
- Energy
- Skill points
- Cooldowns
- Action order
- Active targets
- Spawned enemies

Avoid scattered global variables when a structured state object is appropriate.

---

## 26. Timeline

Timeline entries should represent actual simulation events.

Examples:
- Character action
- Enemy action
- Damage event
- Buff applied
- Debuff applied
- Action Advance
- Action Delay
- Enemy spawn
- Enemy death
- Wave transition

Avoid visual-only guesses.

---

## 27. Waves

Do not assume all waves behave identically.

When implementing multi-wave combat:
- Track wave transitions explicitly.
- Reset or preserve state according to the intended mechanic.
- Do not carry enemy instances into a new wave accidentally.
- Do not assume action ordering remains identical across waves.

---

## 28. Speed Breakpoints

Speed breakpoints should generally be informational calculations rather than absolute guarantees.

Do not hardcode statements such as:

```text
134 speed = guaranteed two actions
```

without considering the relevant wave/action circumstances.

If displaying breakpoints, make clear that they depend on simulation conditions.

---

## 29. Character-Specific Mechanics

Some characters have mechanics that cannot safely be represented by generic speed/AV logic.

Examples:
- Action Advance
- Action Delay
- Speed locking
- Special action ordering
- Follow-up attacks
- Counter attacks
- Extra turns
- Summons
- Special base-speed behavior

Do not force every mechanic into generic logic if that produces incorrect behavior.

---

## 30. Testing

Whenever practical, add or update tests for changed mechanics.

Prioritize tests for:
- Damage multipliers
- DEF calculations
- RES calculations
- RES PEN
- Vulnerability
- EHR
- Speed
- AV
- Action Advance
- Mid-turn speed changes
- Enemy targeting
- Enemy spawning
- Multi-enemy slots
- Wave transitions

For numerical formulas, include expected values.

Prefer deterministic tests.

---

## 31. Compilation

After modifying C++ code, compile the project.

Windows:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Run:

```powershell
.\build\ui\Release\hsr_ui.exe
```

If CMake configuration is stale:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build
cmake --build build --config Release
```

Do not delete `build/` unless necessary.

---

## 32. Linux Build

```bash
set -e

sudo apt-get update

sudo apt-get install -y     git     cmake     ninja-build     g++     make

cmake -S . -B build -G Ninja
cmake --build build --parallel
```

Do not modify the setup script merely to make a local build pass unless the setup script itself is the task.

---

## 33. Compiler Compatibility

The project should compile cleanly under normal Windows/MSVC builds.

Be careful with C++ brace initialization.

For example:

```cpp
Rectangle r = {50, 20, GetScreenWidth() - 100, 60};
```

may cause MSVC narrowing errors when integers are converted to floats.

Prefer:

```cpp
Rectangle r = {
    50.0f,
    20.0f,
    static_cast<float>(GetScreenWidth() - 100),
    60.0f
};
```

Do not globally rewrite unrelated code to fix one compiler error.

---

## 34. UI Development

The UI uses Raylib.

When changing UI:
1. Inspect the existing screen architecture.
2. Reuse existing styles/components where possible.
3. Preserve existing navigation.
4. Avoid introducing a second UI pattern for the same purpose.
5. Keep UI state separate from simulation state when possible.
6. Verify the application still builds.

Do not redesign the entire UI unless explicitly requested.

---

## 35. Enemy UI

The enemy selection screen uses five slots.

Maintain:
- Exactly five slots.
- Multiple enemy IDs per slot.
- Empty slots are valid.
- The simulation receives the same structure the UI displays.

Do not silently limit a slot to one enemy.

---

## 36. Error Handling

Prefer explicit error handling.

Do not silently ignore:
- Missing enemy IDs
- Invalid character IDs
- Missing data
- Invalid stats
- Impossible simulation states
- Failed asset loads
- Invalid configuration

When appropriate, return an error or display a useful diagnostic.

---

## 37. Data Files

Important data lives under:

```text
engine/hsr_engine/data/
```

Treat data files as application input.

Do not randomly modify large data files to solve code bugs.

Before changing a data file:
1. Verify that the data itself is incorrect.
2. Identify how the application consumes it.
3. Determine whether the bug is actually in parser/calculation code.

---

## 38. JSON / CSV

When working with JSON or CSV:
- Preserve existing schemas.
- Avoid changing field names unnecessarily.
- Preserve compatibility with existing data.
- Validate syntax after modifications.
- Do not reformat huge files unless necessary.

If a schema change is required, update all relevant consumers.

---

## 39. Performance

Do not prematurely optimize.

Prioritize:

```text
Correctness
→ Maintainability
→ Testability
→ Performance
```

Avoid obviously expensive operations such as repeatedly scanning huge asset directories during normal simulation logic.

Do not load large visual assets merely to calculate damage.

---

## 40. Git Safety

NEVER run destructive Git commands unless explicitly requested.

Do NOT run:

```bash
git reset --hard
git clean -fd
git checkout -- .
git restore .
```

Do not delete user changes.

Do not overwrite modified files simply to make the repository clean.

Before potentially destructive operations, stop and ask.

---

## 41. Git Add Safety

Do NOT blindly run:

```bash
git add .
```

The repository may contain:
- Local build output
- Backup directories
- Temporary files
- Experimental changes
- Generated files

If committing is explicitly requested:
1. Inspect `git status`.
2. Inspect the diff.
3. Stage only intended files.
4. Review the staged diff.
5. Commit.

---

## 42. Commits

Do not create commits unless explicitly asked.

Do not push unless explicitly asked.

Use concise descriptive commit messages, for example:

```text
Implement RES PEN damage calculation
```

or:

```text
Fix enemy slot selection
```

Avoid vague messages such as:

```text
changes
```

---

## 43. Existing User Changes

Always preserve existing user modifications.

Before making substantial changes, inspect:

```bash
git status
```

If files already contain unrelated modifications, do not overwrite them.

If a requested change overlaps heavily with existing uncommitted work, modify only the required portions.

---

## 44. OpenCode Search Behavior

Do not recursively inspect the entire repository without a reason.

Start with targeted searches:

```text
Search for the relevant class/function.
Inspect its declaration.
Inspect its implementation.
Inspect its callers.
Inspect related data structures.
Then modify.
```

Do not waste context on:

```text
build/
raylib/
ui/assets/
images/
```

unless specifically relevant.

---

## 45. Avoid Context Waste

Large files and generated data can consume significant model context.

When investigating:
- Search for symbols first.
- Open only relevant sections.
- Avoid dumping entire huge files.
- Avoid inspecting binary files.
- Avoid reading hundreds of image filenames unless necessary.
- Avoid recursively reading third-party dependencies.

The objective is to understand the code with the minimum necessary context.

---

## 46. Before Implementing

Before writing code, determine:

1. Where does the existing behavior live?
2. What data structure represents it?
3. Who calls it?
4. What existing behavior must remain unchanged?
5. What is the smallest change that satisfies the request?
6. How will the change be tested?

Do not immediately start rewriting files.

---

## 47. After Implementing

After making a change:

1. Inspect the diff.
2. Check for accidental changes.
3. Compile.
4. Run relevant tests.
5. Fix errors.
6. Re-check the diff.
7. Report what changed.

If a test cannot be run, state that clearly.

Do not claim successful testing without actually testing.

---

## 48. When Requirements Are Ambiguous

Do not invent major requirements.

If a small implementation detail is ambiguous but has an obvious safe default, use the existing project architecture.

If ambiguity could significantly change behavior, ask the user before implementing.

Examples requiring clarification:
- Changing the meaning of an existing stat
- Replacing an existing architecture
- Removing existing mechanics
- Changing save/data formats
- Changing how enemy slots behave
- Introducing a new character-specific rule with unclear behavior

---

## 49. Documentation

When adding a non-obvious mechanic, add a concise comment explaining WHY the code behaves that way.

Avoid comments that merely restate code.

Bad:

```cpp
// Add speed
speed += bonus;
```

Good:

```cpp
// Percentage Speed scales base speed only;
// flat Speed is added afterward.
speed = baseSpeed * (1.0f + speedPercent) + flatSpeed;
```

---

## 50. No Unnecessary Dependencies

Do not add external dependencies unless necessary.

Before adding a library:
1. Check whether the functionality already exists.
2. Check whether C++ standard library functionality is sufficient.
3. Check existing project dependencies.
4. Consider build implications on Windows and Linux.

---

## 51. Production Code vs Test Code

Clearly separate temporary experiments from production implementation.

Do not leave:
- Debug prints
- Temporary hardcoded values
- Fake damage
- Experimental branches of logic
- Commented-out obsolete code

in production code after the task is complete.

---

## 52. Current Development Priority

When deciding what to work on next, prioritize:

1. Correct damage formulas
2. Sections 10–14 of the damage specification
3. Speed and AV mechanics
4. Action Advance
5. Mid-turn speed changes
6. Character/loadout input model
7. Simulation engine integration
8. Enemy spawning and multi-enemy behavior
9. Wave behavior
10. Full combat mechanics
11. Optimization

Do not optimize the simulator before the underlying combat mechanics are reliable.

---

## 53. Important Principle

The project is a simulator, not merely a UI calculator.

The long-term architecture should allow:

```text
Character configuration
        ↓
Stats / Loadout
        ↓
Combat state
        ↓
Action selection
        ↓
Action execution
        ↓
Damage calculation
        ↓
Buff / debuff changes
        ↓
Enemy state changes
        ↓
Timeline
        ↓
Cycle result
```

Keep this architecture in mind when implementing new systems.

---

## 54. Final Rule

When in doubt:

```text
Inspect first.
Change minimally.
Preserve existing behavior.
Keep mechanics data-driven.
Avoid generated/vendor files.
Compile after changes.
Test when possible.
Never hide failures.
Never destroy user work.
```
