# Benchmark Report: Raylib vs SFML (2D sprite loop)

**Date**: 2025-11-21 | **Status**: Draft

## Purpose

Compare minimal 2D render/update loops across Raylib and SFML. Four benches mirror the same workload (OOP and ECS styles). Two extra “readable” demos show the minimum code to open a window and draw one sprite.

## Targets

- `raylib_oop_bench`, `raylib_ecs_bench`
- `sfml_oop_bench`, `sfml_ecs_bench`
- Demos: `raylib_readable_demo`, `sfml_readable_demo`

## How to Run

From repo root (ensures `assets/ship.png` resolves):

```bash
# build everything
xmake -y -P benchmarks/render/01_raylib_vs_sfml

# run benches (default: 500 entities, 180 frames)
xmake -P benchmarks/render/01_raylib_vs_sfml run raylib_oop_bench
xmake -P benchmarks/render/01_raylib_vs_sfml run raylib_ecs_bench
xmake -P benchmarks/render/01_raylib_vs_sfml run sfml_oop_bench
xmake -P benchmarks/render/01_raylib_vs_sfml run sfml_ecs_bench

# run simple window demos
xmake -P benchmarks/render/01_raylib_vs_sfml run raylib_readable_demo
xmake -P benchmarks/render/01_raylib_vs_sfml run sfml_readable_demo
```

Adjust workload: `--entities 1000 --frames 240`. Disable `ASSET_ROOT` or run from repo root to use the ship texture; fallback is a 32x32 blue square.

## Notes on Fidelity

- SFML benches render to an offscreen `RenderTexture` (stability on macOS); Raylib draws to a window. Backends differ, so raw timings are not 1:1.
- `sfml_oop_bench` recreates a sprite per entity per frame; Raylib draws directly from a texture handle. This adds overhead to SFML beyond the API cost.
- Vsync is off; wall-clock includes update + draw. Results vary by GPU/driver/OS.

## Making Results Comparable (next steps)

- Align draw paths: reuse a sprite in SFML OOP; consider window vs offscreen parity for both.
- Pin build mode (`-m release`), hardware/OS, and driver versions in the report.
- Run multiple trials; report mean/stddev. Optionally capture CPU time and memory.
- Verify correctness: ensure both pipelines draw identical entity counts/positions.

## Current Output (example)

Raylib demo run on Apple M2 (fallback texture):
```
raylib_readable_demo entities=500 frames=180 total_ms=1545.49 per_frame_ms=8.58607
```
No comparable SFML timing captured yet on this machine due to macOS backend constraints.
