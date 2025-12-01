# Raylib vs SFML (OOP vs ECS)

Six runnable benches/demos:
- `raylib_oop_bench`
- `raylib_ecs_bench`
- `raylib_readable_demo` (minimal Raylib-only workload to highlight brevity/readability)
- `sfml_oop_bench`
- `sfml_ecs_bench`
- `sfml_readable_demo` (minimal SFML-only workload to mirror the Raylib demo)

Each draws N sprites for F frames with a fixed dt, updates positions, and prints wall time and per-frame cost.

## Build

From repo root:
```bash
cd benchmarks/render/01_raylib_vs_sfml
xmake -y -P . raylib_oop_bench raylib_ecs_bench raylib_readable_demo sfml_oop_bench sfml_ecs_bench
xmake -y -P . sfml_readable_demo
```

## Run

Run from repo root so `assets/ship.png` is found:
```bash
xmake -P benchmarks/render/01_raylib_vs_sfml run raylib_oop_bench
xmake -P benchmarks/render/01_raylib_vs_sfml run raylib_ecs_bench
xmake -P benchmarks/render/01_raylib_vs_sfml run raylib_readable_demo
xmake -P benchmarks/render/01_raylib_vs_sfml run sfml_oop_bench
xmake -P benchmarks/render/01_raylib_vs_sfml run sfml_ecs_bench
xmake -P benchmarks/render/01_raylib_vs_sfml run sfml_readable_demo
```

`raylib_readable_demo` is intentionally concise: the same sprite update/draw loop
fits in ~90 lines versus ~120 in `sfml_oop.cpp`.

`sfml_readable_demo` mirrors it using SFML primitives in a similarly short loop.
On macOS we link with `-ObjC` in `xmake.lua` so the Cocoa window backend works.

Adjust entities/frames via args: `--entities 1000 --frames 240`. Defaults: 500 entities, 180 frames.

If `assets/ship.png` is missing, a 32x32 blue fallback texture is used. Set
`ASSET_ROOT` to point to the repo root if running from elsewhere. SFML benches
render into an offscreen `RenderTexture` (avoids macOS window backend issues).
