# HSR 0-Cycle Raylib UI

Initial C++17/Raylib UI shell.

## Current milestone

- Raylib window
- Sidebar navigation shell
- Team Builder screen
- Four team slots
- Character library
- Lazy-loaded character textures from `asset_manifest.csv`
- C++17 filesystem-independent CSV manifest loader
- Placeholder EngineBridge boundary

## Build

From the project root:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\ui\Release\hsr_ui.exe
```

Raylib is expected at:

```text
./raylib
```

Character assets are expected under:

```text
./ui/assets/prydwen_assets/characters
```

The build copies `ui/assets` beside the executable automatically.

## Important

The UI does not calculate HSR combat formulas. The simulator/rules engine remains the source of truth.
