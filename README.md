# SerpentPost (CLI + Qt GUI)

Post-processing of `dep.m` (Serpent). Includes both CLI and graphical interface (Qt 6, Widgets + Charts).

## Current Features

- Parsing `dep.m` (ZAI/NAMES, all MAT_*/TOT_* parameters, BU, DAYS).
- Data model: `steps[step][material][param] = vector<double>`.
- CSV export: one file per material (including `TOT.csv`).
- Logging via `slog::Logger` (spdlog).
- **GUI**:
  - Open a single `dep.m`
  - Select material, parameter, nuclide (`Total` + nuclides without Lost/Total)
  - Tabular view (Step, BU, DAYS, Value)
  - Chart (X: BU/DAYS; Y: Linear / Log10)
  - Export CSV from GUI

## Build (Windows, VS 2022, Qt 6.9.1 msvc2022_64)

```powershell
# In the repository root
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64" `
  -DQt6_DIR="C:\Qt\6.9.1\msvc2022_64\lib\cmake\Qt6"
cmake --build build --config Release
```

> If a MinGW Qt kit is also installed — ensure CMake picks **msvc2022_64** (check `build/CMakeCache.txt`).  
> In `gui/CMakeLists.txt`, link with `Qt6::Core Gui Widgets Charts`.

## Running the GUI

In `build\gui\Release`:
```powershell
# For running outside of Qt environment:
& "C:\Qt\6.9.1\msvc2022_64\bin\windeployqt.exe" --release --compiler-runtime .\SerpentPostGui.exe
.\SerpentPostGui.exe
```

## Usage

1. `Open dep.m` — choose the file.
2. Use dropdowns to select **Material**, **Parameter**, **Nuclide**.
3. Change X-axis (**BU/DAYS**) and Y-scale (**Linear/Log10**).
4. `Export CSV` — save CSV per material to a chosen directory.

### Conventions
- Scalars: `VOLUME`, `BURNUP`, `BU`, `DAYS` → values only in `Total` (Lost column is empty).
- `Niso` — all nuclides except `Lost/Total` (detected via `ZAI 666/0` or `NAMES`).
- Parameters with underscores (`ING_TOX`, `INH_TOX`) are not split.

## Known Issues / TODO

- Logarithmic axis hides points `≤ 0` (invalid for log).  
- Chart zoom/pan — [TODO] (suggest rubber band + mouse wheel).  
- Chart export to PNG/SVG — [TODO].  
- Multiple file support — [TODO] (load several `dep.m` files and plot combined results on one chart).





## Build on Linux (theoretical guide)

> **Note:** Tested on Ubuntu/Debian-based systems.  
> Replace `apt` with your distro's package manager (`dnf`, `pacman`, etc.).

### 1. Install dependencies

Using system packages:
```bash
# Qt 6 (Widgets + Charts), CMake, and compiler
sudo apt update
sudo apt install cmake g++ qt6-base-dev qt6-charts-dev
```

If Qt 6 is not available in your package manager, install from the official Qt site:
```bash
# Download Qt installer
wget https://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
chmod +x qt-unified-linux-x64-online.run
./qt-unified-linux-x64-online.run
# Select Qt 6.x.x (gcc_64) during installation
```

### 2. Clone repository
```bash
git clone https://github.com/username/SerpentPost.git
cd SerpentPost
```

### 3. Configure build
```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/gcc_64"
```
> Adjust `/path/to/Qt/6.x.x/gcc_64` to your actual Qt installation path.

### 4. Compile
```bash
cmake --build build --config Release -j$(nproc)
```

### 5. Run
```bash
./build/gui/SerpentPostGui
```

---

**Notes for Linux:**
- The `windeployqt` step on Windows is not required — Linux dynamically links to system-installed Qt libraries.
- If running from a different environment or deploying to another machine, you may need to use `linuxdeployqt` to bundle Qt dependencies.
- Qt Charts must be installed — in some distros it is a separate package (`qt6-charts-dev` or similar).
