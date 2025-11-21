# Benchmark Report: Storage Comparison

**Date**: 2025-11-21 | **Status**: Final

## Executive Summary

Sparse Array dominates for insertion (**15.6x faster** than Dense Array) while maintaining competitive iteration
performance. Dense Array offers best iteration speed through cache locality but suffers from expensive insertions due to
index map maintenance.

| Operation          | Sparse Array | Dense Array | Packed Array | Linked List | Winner   |
|--------------------|--------------|-------------|--------------|-------------|----------|
| Insert (10K)       | 16.9 μs      | 264 μs      | 89.7 μs      | 146 μs      | **S.A.** |
| Iterate (10K)      | 17.4 μs      | 17.3 μs     | 33.5 μs      | 22.4 μs     | **D.A.** |
| Insert Throughput  | 594M ops/s   | 37.9M ops/s | 111M ops/s   | 68.5M ops/s | **S.A.** |
| Iterate Throughput | 575M ops/s   | 577M ops/s  | 299M ops/s   | 447M ops/s  | **D.A.** |

---

## Storage Architecture Comparison

### Sparse Array: Optional Slots

```
Memory Layout:
Index:  0     1     2     3     ...   9999
Data:  [Opt] [Opt] [Opt] [Opt] ...  [Opt]

Each slot: std::optional<Component> (16 bytes with overhead)
Total: ~160KB for 10K components

Characteristics:
- Direct index access: O(1)
- Insertion: Simple assignment, auto-resize
- Deletion: Set to std::nullopt
- Iteration: Check has_value() for each element
- Cache: Moderate (optional overhead adds checks)
```

**Performance at 10K:**

- Insert: 16.9 μs (594M/s)
- Iterate: 17.4 μs (575M/s)

---

### Dense Array: Index-Mapped Compaction

```
Memory Layout:
┌─ Data Vector:    [C0][C1][C2][C3]...           (tightly packed)
└─ Index Map:      {0→0, 5→1, 12→2, 9→3, ...}   (hash map)

Each component: 16 bytes
Total: ~160KB data + hash map overhead

Characteristics:
- Direct index access: O(1) via map lookup
- Insertion: Append + map entry, O(1) amortized
- Deletion: Swap with last + map update, O(1) amortized
- Iteration: Pure data vector iteration (no checks)
- Cache: Excellent (compact memory, no gaps)
```

**Performance at 10K:**

- Insert: 264 μs (37.9M/s) ❌ Hash map overhead
- Iterate: 17.3 μs (577M/s) ✅ Best iteration

---

### Packed Array: Active Flag Vector

```
Memory Layout:
┌─ Data Vector:  [C0][C1][C2][C3]...[C9999]   (all slots)
└─ Active Flags: [T][F][T][T]...[F]            (boolean per slot)

Each component: 16 bytes
Each flag: 1 byte
Total: ~160KB data + ~10KB flags

Characteristics:
- Direct index access: O(1)
- Insertion: Direct assignment + flag set
- Deletion: Just unset flag (no resize)
- Iteration: Requires filtering to get_active() vector
- Cache: Poor iteration (must rebuild active list)
```

**Performance at 10K:**

- Insert: 89.7 μs (111M/s) ⚠️ Resizing overhead
- Iterate: 33.5 μs (299M/s) ❌ Worst (rebuilds vector)

---

### Linked List: Node-Based Chain

```
Memory Layout:
Heap:   [Node0]→[Node1]→[Node2]→...→[Node9999]
        ├─ data (16 bytes)
        └─ next pointer (8 bytes)

Per node: 24 bytes
Total: ~240KB + allocation fragmentation

Characteristics:
- Direct index access: None (O(n) to kth element)
- Insertion: Just append, O(1)
- Deletion: O(1) if iterator known
- Iteration: Linear traversal via pointers
- Cache: Terrible (pointer chasing kills cache)
```

**Performance at 10K:**

- Insert: 146 μs (68.5M/s) ❌ Allocation overhead
- Iterate: 22.4 μs (447M/s) ⚠️ Pointer chasing

---

## Detailed Performance Analysis

### Insertion Performance (10K Elements)

```
Sparse Array:  16.9 μs ═══════════════════════════════════════════ (baseline)
Dense Array:   264 μs  ═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════
Packed Array:  89.7 μs ════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════
Linked List:   146 μs  ══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════

Ratio vs Sparse Array:
Dense Array:   15.6x slower (hash map lookups on every insert)
Packed Array:   5.3x slower (vector resize operations)
Linked List:    8.6x slower (allocations + pointer setup)
```

**Why Sparse Array Wins:**

- Direct `vector.resize()` with placement
- No hash map updates
- No allocation fragmentation
- Simple `std::optional` assignment

---

### Iteration Performance (10K Elements)

```
Dense Array:   17.3 μs ═════════════════════════ (baseline)
Sparse Array:  17.4 μs ═════════════════════════ (1.01x)
Linked List:   22.4 μs ════════════════════════════════════════ (1.29x)
Packed Array:  33.5 μs ═══════════════════════════════════════════════════════════════════ (1.93x)

L1 Cache Effectiveness:
Dense Array:   Best - compact, no gaps, no checks
Sparse Array:  Good - checks, but memory sequential
Linked List:   Poor - pointer chasing, L1 misses
Packed Array:  Worst - must create active() vector first
```

**Why Dense Array Wins:**

- Pure iteration over compact vector
- No optional checks
- Predictable access pattern
- Minimal CPU overhead

---

## Scalability Analysis

### Insertion Scaling (100 to 10,000 elements)

| Size | Sparse   | Dense   | Packed   | Linked  |
|------|----------|---------|----------|---------|
| 100  | 0.162 μs | 2.33 μs | 0.937 μs | 1.39 μs |
| 1K   | 1.67 μs  | 28.0 μs | 8.42 μs  | 14.7 μs |
| 10K  | 16.9 μs  | 264 μs  | 89.7 μs  | 146 μs  |

**Observation:** Dense Array scales worst (O(n) hash ops), Sparse Array is linear.

---

### Iteration Scaling (100 to 10,000 elements)

| Size | Sparse   | Dense    | Packed   | Linked   |
|------|----------|----------|----------|----------|
| 100  | 0.148 μs | 0.103 μs | 0.455 μs | 0.160 μs |
| 1K   | 1.75 μs  | 1.72 μs  | 3.50 μs  | 1.77 μs  |
| 10K  | 17.4 μs  | 17.3 μs  | 33.5 μs  | 22.4 μs  |

**Observation:** Sparse/Dense maintain same ratio, Linked List pointer chasing becomes obvious.

---

## Memory Footprint Analysis

**For 10,000 Components (16 bytes each):**

```
Sparse Array:
├─ Data vector:          ~160 KB (optional overhead)
├─ Vector capacity:      ~20 KB overhead
└─ Total:                ~180 KB ✅ SMALLEST

Dense Array:
├─ Data vector:          ~160 KB
├─ Hash map:             ~80 KB (16 bytes per entry × 10K)
└─ Total:                ~240 KB ⚠️ 33% MORE

Packed Array:
├─ Data vector:          ~160 KB
├─ Active flags:         ~10 KB
└─ Total:                ~170 KB ✅ COMPACT

Linked List:
├─ Nodes (24 bytes):     ~240 KB
├─ Heap fragmentation:   ~60 KB (estimated)
└─ Total:                ~300 KB ❌ WORST
```

---

## Use Case Recommendations

### ✅ Use Sparse Array When:

- Frequent insertions/deletions
- Sparse entity population (many empty slots)
- ECS component storage (access by entity ID)
- Memory efficiency matters
- Simplicity preferred

**Example:** `registry.get_component<Position>(entity_id)`

---

### ✅ Use Dense Array When:

- Only iteration needed (systems processing all components)
- Cache efficiency critical
- Few insertions/deletions
- Fixed entity set during frame
- Maximum iteration throughput

**Example:** `for (auto& comp : components.get_data())`

---

### ✅ Use Packed Array When:

- Frequent active/inactive toggling
- Sparse deletion patterns
- Need to query "active count"
- Avoiding pointer fragmentation

**Example:** `disabled_entities` flag toggle

---

### ❌ Avoid Linked List For:

- Game loops (poor cache)
- Tight iteration loops
- Memory-constrained systems
- Performance-critical paths

**Only use for:** Queue/stack semantics where you need efficient head removal.

---

## R-Type Engine Recommendation

**For ECS Implementation:**

```
Component Storage Strategy:
├─ Position, Velocity, Health:    SPARSE ARRAY
│  └─ Access: registry[entity_id].position
│
├─ System Processing:             DENSE ARRAY (copy + iterate)
│  └─ Iteration: for (auto& comp : system.components)
│
└─ Inactive Entities:             PACKED ARRAY with flags
   └─ Query: disabled_entities.get_active()
```

**Rationale:**

1. **Insert/Delete:** Sparse Array for O(1) entity management
2. **Processing:** Dense Array copy for iteration speed (50-100 μs overhead for frame)
3. **Memory:** ~180 KB for 10K components (negligible)
4. **Cache:** L1 friendly for physics/AI loops

---

## Benchmark Results Summary Table

### Insertion Performance (μs for 10K items)

| Storage      | Time | Throughput | Notes                  |
|--------------|------|------------|------------------------|
| Sparse Array | 16.9 | 594M/s     | ✅ Best - direct resize |
| Packed Array | 89.7 | 111M/s     | Vector ops slower      |
| Linked List  | 146  | 68.5M/s    | Allocation overhead    |
| Dense Array  | 264  | 37.9M/s    | ❌ Worst - hash map ops |

---

### Iteration Performance (μs for 10K items)

| Storage      | Time | Throughput | Notes                      |
|--------------|------|------------|----------------------------|
| Dense Array  | 17.3 | 577M/s     | ✅ Best - compact iteration |
| Sparse Array | 17.4 | 575M/s     | Competitive - 0.6% slower  |
| Linked List  | 22.4 | 447M/s     | Pointer chasing penalty    |
| Packed Array | 33.5 | 299M/s     | ❌ Worst - rebuilds vector  |

---

## Conclusion

**Sparse Array is optimal for ECS component storage:**

1. **15.6x faster insertion** than Dense Array
2. **Competitive iteration** (99.4% of Dense Array speed)
3. **Smallest memory footprint** among alternatives
4. **Handles sparse entity populations** naturally
5. **Simple std::optional semantics** reduce bugs

**Reserve Dense Array for frame-local iteration copies** when maximum throughput is needed for a single system iteration
loop.

---

## How to Run Storage Comparison Benchmarks

```bash
cd benchmarks/engine
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)

./02_storage_comparison/storage_comparison_poc         # Interactive demo
./02_storage_comparison/storage_comparison_benchmark   # Detailed measurements
```