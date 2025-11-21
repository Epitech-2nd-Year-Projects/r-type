#include <chrono>
#include <iomanip>
#include <iostream>
#include "storage.h"

void RunSparseArrayDemo() {
  std::cout << "SPARSE ARRAY (1000 components, 100 iterations)\n";
  std::cout
      << "──────────────────────────────────────────────────────────────\n";

  SparseArrayStorage<Component> storage;

  for (int i = 0; i < 1000; ++i) {
    storage.insert(i, {static_cast<float>(i), 2.0f, 100});
  }

  auto start = std::chrono::high_resolution_clock::now();

  for (int iter = 0; iter < 100; ++iter) {
    volatile float sum = 0;
    for (auto& opt : storage) {
      if (opt.has_value()) {
        sum += opt.value().x;
      }
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  std::cout << "Components:      " << storage.size() << "\n";
  std::cout << "Iterations:      100\n";
  std::cout << "Total time:      " << std::fixed << std::setprecision(2)
            << duration_us / 1000.0 << " ms\n";
  std::cout << "Per iteration:   " << duration_us / 100.0 / 1000.0 << " ms\n";
  std::cout << "\n";
}

void RunDenseArrayDemo() {
  std::cout << "DENSE ARRAY (1000 components, 100 iterations)\n";
  std::cout
      << "──────────────────────────────────────────────────────────────\n";

  DenseArrayStorage<Component> storage;

  for (int i = 0; i < 1000; ++i) {
    storage.insert(i, {static_cast<float>(i), 2.0f, 100});
  }

  auto start = std::chrono::high_resolution_clock::now();

  for (int iter = 0; iter < 100; ++iter) {
    volatile float sum = 0;
    for (const auto& comp : storage) {
      sum += comp.x;
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  std::cout << "Components:      " << storage.size() << "\n";
  std::cout << "Iterations:      100\n";
  std::cout << "Total time:      " << std::fixed << std::setprecision(2)
            << duration_us / 1000.0 << " ms\n";
  std::cout << "Per iteration:   " << duration_us / 100.0 / 1000.0 << " ms\n";
  std::cout << "\n";
}

void RunPackedArrayDemo() {
  std::cout << "PACKED ARRAY (1000 components, 100 iterations)\n";
  std::cout
      << "──────────────────────────────────────────────────────────────\n";

  PackedArrayStorage<Component> storage;

  for (int i = 0; i < 1000; ++i) {
    storage.insert(i, {static_cast<float>(i), 2.0f, 100});
  }

  auto start = std::chrono::high_resolution_clock::now();

  for (int iter = 0; iter < 100; ++iter) {
    auto active = storage.get_active();
    volatile float sum = 0;
    for (const auto& comp : active) {
      sum += comp.x;
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  std::cout << "Components:      " << storage.active_count() << "\n";
  std::cout << "Iterations:      100\n";
  std::cout << "Total time:      " << std::fixed << std::setprecision(2)
            << duration_us / 1000.0 << " ms\n";
  std::cout << "Per iteration:   " << duration_us / 100.0 / 1000.0 << " ms\n";
  std::cout << "\n";
}

void RunLinkedListDemo() {
  std::cout << "LINKED LIST (1000 components, 100 iterations)\n";
  std::cout
      << "──────────────────────────────────────────────────────────────\n";

  LinkedListStorage<Component> storage;

  for (int i = 0; i < 1000; ++i) {
    storage.insert(i, {static_cast<float>(i), 2.0f, 100});
  }

  auto start = std::chrono::high_resolution_clock::now();

  for (int iter = 0; iter < 100; ++iter) {
    volatile float sum = 0;
    for (const auto& comp : storage) {
      sum += comp.x;
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  std::cout << "Components:      " << storage.size() << "\n";
  std::cout << "Iterations:      100\n";
  std::cout << "Total time:      " << std::fixed << std::setprecision(2)
            << duration_us / 1000.0 << " ms\n";
  std::cout << "Per iteration:   " << duration_us / 100.0 / 1000.0 << " ms\n";
  std::cout << "\n";
}

int main() {
  RunSparseArrayDemo();
  RunDenseArrayDemo();
  RunPackedArrayDemo();
  RunLinkedListDemo();

  return 0;
}
