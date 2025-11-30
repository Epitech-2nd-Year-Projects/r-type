#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../.." && pwd)"

targets=(
  raylib_oop_bench
  raylib_ecs_bench
  sfml_oop_bench
  sfml_ecs_bench
)

entities="${ENTITIES:-500}"
frames="${FRAMES:-180}"

echo "Building targets (raylib/sfml will be fetched if missing)..."
for t in "${targets[@]}"; do
  xmake -y -P "${script_dir}" "${t}"
done

cd "${repo_root}"
for t in "${targets[@]}"; do
  echo "Running ${t} (entities=${entities}, frames=${frames})"
  ASSET_ROOT="${repo_root}" xmake run -P "${script_dir}" "${t}" -- --entities "${entities}" --frames "${frames}"
done

echo "Done."
