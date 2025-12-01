# Xmake vs CMake Build PoC

Purpose: compare configure/full rebuild/incremental times on the same tiny C++
project. Uses `fmt` to illustrate dependency management. CMake expects vcpkg
(`VCPKG_ROOT` set) to find `fmt`; xmake pulls `fmt` via `add_requires("fmt")`.

## Layout

- `src/`: two translation units (`main.cpp`, `foo.cpp`) so incremental rebuilds
  have a measurable effect.
- `xmake.lua`: builds `build_poc` with xmake.
- `CMakeLists.txt`: builds the same target with CMake.
- `vcpkg.json`: manifest so CMake can fetch `fmt` via vcpkg.
- `bench.sh`: helper script to time configure + full + incremental for both.

## Quick run

```bash
cd benchmarks/build/01_xmake_vs_cmake
bash bench.sh
```

Outputs land under `build_xmake/` and `build_cmake/`. The script prints
wall-clock timings using `/usr/bin/time -p`.

## Manual commands (if you prefer)

- Xmake configure: `/usr/bin/time -p xmake f -P . -o build_xmake -m release -c`
- Xmake full build: `/usr/bin/time -p xmake -P . -r`
- CMake configure (requires `cmake` and `VCPKG_ROOT`):  
  `/usr/bin/time -p cmake -S . -B build_cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake`
- CMake full build: `/usr/bin/time -p cmake --build build_cmake -j`

Incremental test idea: `touch src/foo.cpp` then rebuild in each system and time
again. Clean with `rm -rf build_xmake build_cmake`.
