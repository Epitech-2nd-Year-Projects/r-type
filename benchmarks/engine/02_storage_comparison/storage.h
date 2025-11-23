#ifndef BENCH_STORAGE_H
#define BENCH_STORAGE_H

#include <list>
#include <optional>
#include <unordered_map>
#include <vector>

struct Component {
  float x;
  float y;
  int health;
};

template <typename T>
class SparseArrayStorage {
 private:
  std::vector<std::optional<T>> data_;

 public:
  void insert(size_t idx, const T& value) {
    if (idx >= data_.size()) data_.resize(idx + 1);
    data_[idx] = value;
  }

  void erase(size_t idx) {
    if (idx < data_.size()) data_[idx] = std::nullopt;
  }

  void clear() { data_.clear(); }

  std::optional<T>& at(size_t idx) {
    if (idx >= data_.size()) data_.resize(idx + 1);
    return data_[idx];
  }

  size_t size() const { return data_.size(); }

  auto begin() { return data_.begin(); }
  auto end() { return data_.end(); }
};

template <typename T>
class DenseArrayStorage {
 private:
  std::vector<T> data_;
  std::unordered_map<size_t, size_t> index_map_;
  size_t next_id = 0;

 public:
  void insert(size_t idx, const T& value) {
    index_map_[idx] = data_.size();
    data_.push_back(value);
    next_id = std::max(next_id, idx + 1);
  }

  void erase(size_t idx) {
    auto it = index_map_.find(idx);
    if (it != index_map_.end()) {
      size_t pos = it->second;
      if (pos < data_.size() - 1) {
        data_[pos] = data_.back();
        for (auto& p : index_map_) {
          if (p.second == data_.size() - 1) p.second = pos;
        }
      }
      data_.pop_back();
      index_map_.erase(it);
    }
  }

  void clear() {
    data_.clear();
    index_map_.clear();
    next_id = 0;
  }

  std::vector<T>& get_data() { return data_; }
  const std::vector<T>& get_data() const { return data_; }

  size_t size() const { return data_.size(); }

  auto begin() { return data_.begin(); }
  auto end() { return data_.end(); }
};

template <typename T>
class PackedArrayStorage {
 private:
  std::vector<T> data_;
  std::vector<bool> active_;

 public:
  void insert(size_t idx, const T& value) {
    if (idx >= active_.size()) {
      active_.resize(idx + 1, false);
      data_.resize(idx + 1);
    }
    data_[idx] = value;
    active_[idx] = true;
  }

  void erase(size_t idx) {
    if (idx < active_.size()) active_[idx] = false;
  }

  void clear() {
    data_.clear();
    active_.clear();
  }

  std::vector<T> get_active() const {
    std::vector<T> result;
    for (size_t i = 0; i < data_.size(); ++i) {
      if (active_[i]) result.push_back(data_[i]);
    }
    return result;
  }

  size_t size() const { return data_.size(); }
  size_t active_count() const {
    size_t count = 0;
    for (bool b : active_) {
      if (b) count++;
    }
    return count;
  }
};

template <typename T>
class LinkedListStorage {
 private:
  std::list<T> data_;

 public:
  void insert(size_t idx, const T& value) { data_.push_back(value); }

  void erase(size_t idx) {
    if (!data_.empty()) data_.pop_front();
  }

  void clear() { data_.clear(); }

  size_t size() const { return data_.size(); }

  auto begin() { return data_.begin(); }
  auto end() { return data_.end(); }
};

#endif