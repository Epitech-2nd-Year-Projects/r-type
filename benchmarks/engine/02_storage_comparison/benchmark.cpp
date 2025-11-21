#include "storage.h"
#include <benchmark/benchmark.h>

static void BM_SparseArray_Insert(benchmark::State& state) {
  const int count = state.range(0);
  SparseArrayStorage<Component> storage;

  for (auto _ : state) {
    storage.clear();
    for (int i = 0; i < count; ++i) {
      storage.insert(i, {static_cast<float>(i), 2.0f, 100});
    }
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_SparseArray_Iterate(benchmark::State& state) {
  const int count = state.range(0);
  SparseArrayStorage<Component> storage;

  for (int i = 0; i < count; ++i) {
    storage.insert(i, {static_cast<float>(i), 2.0f, 100});
  }

  for (auto _ : state) {
    volatile float sum = 0;
    for (auto& opt : storage) {
      if (opt.has_value()) {
        sum += opt.value().x;
      }
    }
    benchmark::DoNotOptimize(sum);
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_DenseArray_Insert(benchmark::State& state) {
  const int count = state.range(0);
  DenseArrayStorage<Component> storage;

  for (auto _ : state) {
    storage = DenseArrayStorage<Component>();
    for (int i = 0; i < count; ++i) {
      storage.insert(i, {static_cast<float>(i), 2.0f, 100});
    }
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_DenseArray_Iterate(benchmark::State& state) {
  const int count = state.range(0);
  DenseArrayStorage<Component> storage;

  for (int i = 0; i < count; ++i) {
    storage.insert(i, {static_cast<float>(i), 2.0f, 100});
  }

  for (auto _ : state) {
    volatile float sum = 0;
    for (const auto& comp : storage) {
      sum += comp.x;
    }
    benchmark::DoNotOptimize(sum);
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_PackedArray_Insert(benchmark::State& state) {
  const int count = state.range(0);
  PackedArrayStorage<Component> storage;

  for (auto _ : state) {
    storage = PackedArrayStorage<Component>();
    for (int i = 0; i < count; ++i) {
      storage.insert(i, {static_cast<float>(i), 2.0f, 100});
    }
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_PackedArray_Iterate(benchmark::State& state) {
  const int count = state.range(0);
  PackedArrayStorage<Component> storage;

  for (int i = 0; i < count; ++i) {
    storage.insert(i, {static_cast<float>(i), 2.0f, 100});
  }

  for (auto _ : state) {
    auto active = storage.get_active();
    volatile float sum = 0;
    for (const auto& comp : active) {
      sum += comp.x;
    }
    benchmark::DoNotOptimize(sum);
  }

  state.SetItemsProcessed(state.iterations() * storage.active_count());
}

static void BM_LinkedList_Insert(benchmark::State& state) {
  const int count = state.range(0);
  LinkedListStorage<Component> storage;

  for (auto _ : state) {
    storage = LinkedListStorage<Component>();
    for (int i = 0; i < count; ++i) {
      storage.insert(i, {static_cast<float>(i), 2.0f, 100});
    }
  }

  state.SetItemsProcessed(state.iterations() * count);
}

static void BM_LinkedList_Iterate(benchmark::State& state) {
  const int count = state.range(0);
  LinkedListStorage<Component> storage;

  for (int i = 0; i < count; ++i) {
    storage.insert(i, {static_cast<float>(i), 2.0f, 100});
  }

  for (auto _ : state) {
    volatile float sum = 0;
    for (const auto& comp : storage) {
      sum += comp.x;
    }
    benchmark::DoNotOptimize(sum);
  }

  state.SetItemsProcessed(state.iterations() * count);
}

BENCHMARK(BM_SparseArray_Insert)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_SparseArray_Iterate)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_DenseArray_Insert)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_DenseArray_Iterate)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_PackedArray_Insert)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_PackedArray_Iterate)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_LinkedList_Insert)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_LinkedList_Iterate)
    ->RangeMultiplier(2)
    ->Range(100, 10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();