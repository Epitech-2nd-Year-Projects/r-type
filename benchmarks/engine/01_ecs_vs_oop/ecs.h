#ifndef BENCH_ECS_H
#define BENCH_ECS_H

#include <functional>
#include <optional>
#include <vector>

struct Position {
  float x, y;
};

struct Velocity {
  float vx, vy;
};

struct Health {
  int hp;
  int max_hp;
};

template <typename T>
class SparseArray {
 private:
  std::vector<std::optional<T>> data_;

 public:
  std::optional<T> &operator[](size_t idx) {
    if (idx >= data_.size()) data_.resize(idx + 1);
    return data_[idx];
  }

  const std::optional<T> &operator[](size_t idx) const { return data_[idx]; }

  auto begin() { return data_.begin(); }
  auto end() { return data_.end(); }
  size_t size() const { return data_.size(); }
};

class ECSGameWorld {
 public:
  SparseArray<Position> positions;
  SparseArray<Velocity> velocities;
  SparseArray<Health> healths;

  void AddSystem(std::function<void(ECSGameWorld &)> system);
  void Update(float dt);
  size_t EntityCount() const;

 private:
  std::vector<std::function<void(ECSGameWorld &)>> systems_;
};

#endif
