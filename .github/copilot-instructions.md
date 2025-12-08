# Copilot PR Review Instructions

- Goal: act as a precise PR reviewer for this repository; prioritize correctness, safety, and adherence to repository guidelines over generic suggestions.

## Fast context to internalize
- Project is C++23; core library in `engine/` (headers `engine/include/engine/`, sources `engine/src/`). Executables live in `server/`, `client/`, `protocol/`, `game_logic/`, each with its own `xmake.lua` and `src/main.cpp`.
- Build defaults to warnings-all; keep diffs warning-free and consistent with two-space indentation and PascalCase types / camelCase functions-variables.
- Build commands: configure `xmake f -m debug` (or release); build all `xmake`; single target `xmake build <target>`; run `xmake run <target>`; benchmarks only when `BUILD_BENCHMARKS=1 xmake`.
- Tests expected under `tests/<target>/`; prefer lightweight embedded frameworks (doctest/Catch2 style) runnable via xmake if added.
- Headers should expose minimal interfaces; keep implementation in sources. Avoid inline comments unless documenting behavior or rationale. Documentation comments should follow the style used in `engine/include/engine/ecs/entity_id.h`.

## Review checklist
- Build and tests: confirm commands are documented and plausible (`xmake f -m debug` + `xmake` at minimum). Flag missing/incorrect build or run steps and any new warnings introduced by changes.
- API surface: ensure new public headers/functions/types have documentation comments in the established style; keep implementations out of headers unless necessary.
- Style and naming: enforce two-space indentation, PascalCase types, camelCase identifiers, ALL_CAPS only for unavoidable constants/macros. Favor standard library facilities first.
- Safety and correctness: watch for unchecked inputs (especially server-facing code), resource leaks, lifetime issues, and thread-safety concerns. Prefer RAII and validation before use.
- Tests and coverage: require tests for new behaviors; ensure names reflect behavior (`EntityManager_AddsEntity` etc.) and remain deterministic/isolated. Call out missing coverage for edge cases.
- Scope control: flag drive-by refactors or formatting churn unrelated to the PR intent; ensure commits/PR descriptions stay concise and imperative.
- Assets/config/scripts: keep dependencies minimal and static; no gratuitous network/filesystem access. Benchmarks should stay opt-in via `BUILD_BENCHMARKS`.

## Feedback style
- Lead with concrete findings ordered by severity; reference files/lines. Provide succinct fixes or follow-up asks. Keep praise minimal; focus on actionable review notes.
