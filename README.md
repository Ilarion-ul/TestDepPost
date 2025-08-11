# TestDepPost

A small C++17 CLI utility to parse Serpent2 `dep.m` burnup output and export
per-material CSV tables ready for further post-processing and plotting.

> Current version: **v0.1.0**  
> Status: parses ZAI/NAMES, MAT\_/TOT\_ matrices, handles scalars (VOLUME, BURNUP, BU, DAYS), and writes CSV files per material (including `TOT`).

---

## Features

- **Serpent2 `dep.m` parser**
  - Reads `ZAI`, `NAMES`
  - Collects `MAT_<Material>_<PARAM>(:,idx) = [ ... ]` and `TOT_<PARAM>(:,idx) = [ ... ]`
    - Supported params: `ADENS, MDENS/MASS, A, H, SF, GSRC, ING_TOX, INH_TOX, VOLUME, BURNUP`
  - Reads **burnup grid**: `BU = [ ... ]` and **time grid**: `DAYS = [ ... ]`

- **Robust data model**
  - Aggregates by burnup step (`idx`) into:
    ```
    steps[step][material][param] = std::vector<double>
    ```
  - Isotopic vectors include *real isotopes* plus optional **Lost** and **Total**
  - Scalar params (`VOLUME`, `BURNUP`, `BU`, `DAYS`) are stored per step as single values

- **CSV export per material**
  - One CSV per material (including `TOT.csv`)
  - Column order: `step, param, <isotopes...>, Lost, Total`
  - For scalar params, **only `Total`** is filled (Lost is empty by design)

---

## Build

### Windows (MSVC / Visual Studio 2022)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

## Usage

```powershell

.\build\Release\SerpentPostCli.exe <dep.m> <out_dir>

This will produce:

out_csv/TOT.csv

out_csv/<Material1>.csv

out_csv/<Material2>.csv