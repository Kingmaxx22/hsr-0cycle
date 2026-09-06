# HSR 0-Cycle Simulator

A C++/Raylib simulator and team-building tool for Honkai: Star Rail 0-cycle testing.

## Requirements

**Windows**

- Windows 10 or newer
- Git
- CMake 3.20 or newer
- A C++17 compatible compiler
- Visual Studio 2022 or another compiler supported by CMake

The project uses C++17 and builds Raylib from the `raylib` directory included with the repository.

## Download

Clone the repository with Git:

```bash
git clone https://github.com/Kingmaxx22/hsr-0cycle.git
cd hsr-0cycle
```

If you do not have Git installed, open the repository page on GitHub and select:

```
Code -> Download ZIP
```

Extract the ZIP and open a terminal in the extracted `hsr-0cycle` directory.

Make sure the `raylib` directory is present in the project root.

The project should have a structure similar to:

```
hsr-0cycle/
├── CMakeLists.txt
├── raylib/
├── engine/
│   └── hsr_engine/
├── ui/
│   ├── assets/
│   ├── src/
│   └── CMakeLists.txt
└── ...
```

## Build with CMake

From the project root:

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

After the build finishes, the executable will be located in the generated build directory.

For a Visual Studio generator, it will normally be located at:

```
build/ui/Release/hsr_ui.exe
```

Depending on the CMake generator and version being used, the exact output directory may be different.

## Running the Program

Run the generated `hsr_ui.exe`.

The application requires its assets and game data to be available relative to the executable.

The CMake configuration automatically copies the UI assets into the executable's output directory after building.

If the application cannot find its data files, make sure the following directories exist in the repository:

```
engine/hsr_engine/data/
ui/assets/
```

## Visual Studio

You can also generate a Visual Studio solution with CMake.

From the project root:

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
```

Then build with:

```bash
cmake --build . --config Release
```

Or open the generated Visual Studio solution from the build directory.

## Clean Build

If you run into build problems, remove the build directory and configure the project again.

From the project root:

```bash
rmdir /s /q build
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

If using PowerShell:

```powershell
Remove-Item -Recurse -Force build
New-Item -ItemType Directory build
Set-Location build
cmake ..
cmake --build . --config Release
```

## Current Features

- Team building
- Character selection
- Light Cone selection
- Relic management
- Enemy selection
- MoC enemy setup
- Five fixed enemy battlefield positions
- Multiple enemy IDs can be assigned to each position
- Enemy spawn sequences can be represented by adding multiple IDs to a slot
- Search and filtering for enemies
- Enemy statistics and weaknesses display

## Enemy Slot System

The enemy setup currently supports five fixed battlefield positions.

Each position can contain multiple enemy IDs.

For example:

```
Slot 1:
    Enemy A
    Enemy B
    Enemy C

Slot 2:
    Enemy D

Slot 3:
    Enemy E
    Enemy F

Slot 4:
    Empty

Slot 5:
    Enemy G
```

This allows a battlefield position to represent enemies that are replaced or spawned during an encounter.

- Selecting a slot makes it the active slot.
- Selecting an enemy and pressing **ADD SELECTED TO SLOT** adds that enemy to the active slot.
- **REMOVE LAST** removes the most recently added enemy from the active slot.
- **CLEAR SLOT** removes all enemies from the active slot.

The active slot's enemy list is displayed in the top-right panel.

## Project Structure

```
engine/
    hsr_engine/
        data/

ui/
    assets/
    src/
        assets/
        data/
        screens/
        widgets/

raylib/

CMakeLists.txt
```

## Troubleshooting

**CMake cannot find Raylib**

Make sure the repository contains:

```
raylib/
```

in the project root. The root `CMakeLists.txt` expects Raylib to be located at:

```
./raylib
```

**Compiler errors**

Make sure your compiler supports C++17. You can check the compiler version with:

```bash
g++ --version
```

or:

```bash
clang++ --version
```

For Visual Studio:

```bash
cl
```

The project requires C++17.

**CMake version error**

Check your CMake version:

```bash
cmake --version
```

The project requires CMake 3.20 or newer.

**Missing assets**

Make sure you have cloned or extracted the complete repository rather than only downloading individual source files.

The project requires the contents of:

```
ui/assets/
engine/hsr_engine/data/
```

## Development

The project uses CMake as its build system.

To create a development build:

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```

To create a release build:

```bash
cmake --build . --config Release
```

## Contributing

Pull requests and improvements are welcome.

When submitting changes:

- Keep changes focused on the feature or bug being addressed.
- Make sure the project still builds with CMake.
- Test the application before submitting the pull request.
- Avoid committing generated build files or binaries.

## License

This project does not currently have a license.
