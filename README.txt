Relic dropdown update for hsr-0cycle.

Changes:
- Body, Feet, Planar Sphere, and Link Rope main stats are true dropdowns.
- Every substat type is a dropdown.
- Every selected substat's value is a dropdown.
- Substat choices exclude the piece's main stat and duplicate substats.
- Feet includes Break Effect as a legal main stat.
- Substat value dropdowns use 5-star low/mid/high single-roll values.
- Existing custom saved values are preserved.

Install:
1. Copy this folder to the project, or run install-relic-dropdowns.ps1 from this folder.
2. Reconfigure/build:
   cmake -S . -B build
   cmake --build build --config Release
