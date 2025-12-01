#!/usr/bin/env bash
set -u

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$root"

run_time() {
  echo ">> $*"
  /usr/bin/time -p "$@"
  echo
}

echo "=== Xmake ==="
run_time xmake f -P . -o build_xmake -m release -c
run_time xmake -P . -r

echo "=== CMake (requires cmake + VCPKG_ROOT) ==="
if command -v cmake >/dev/null 2>&1; then
  if [ -n "${VCPKG_ROOT:-}" ]; then
    TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    run_time cmake -S . -B build_cmake -DCMAKE_BUILD_TYPE=Release "${TOOLCHAIN}"
    run_time cmake --build build_cmake -j
  else
    echo "skip: VCPKG_ROOT not set; set it to your vcpkg path to compare CMake"
  fi
else
  echo "skip: cmake not found; install cmake to compare"
fi

echo "=== Incremental (touch src/foo.cpp) ==="
touch src/foo.cpp
run_time xmake -P .
if command -v cmake >/dev/null 2>&1 && [ -n "${VCPKG_ROOT:-}" ]; then
  run_time cmake --build build_cmake -j
fi

echo "Done. Builds are in build_xmake/ and build_cmake/."
