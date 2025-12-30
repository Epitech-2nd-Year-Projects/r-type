#ifndef ENGINE_PROFILING_RING_BUFFER_H_
#define ENGINE_PROFILING_RING_BUFFER_H_

#include <array>
#include <cstddef>

namespace engine::profiling {

template <typename T, std::size_t N>
class RingBuffer {
 public:
  void Push(T value) {
    data_[write_index_] = value;
    write_index_ = (write_index_ + 1) % N;
    if (count_ < N) {
      ++count_;
    }
  }

  void Clear() {
    write_index_ = 0;
    count_ = 0;
  }

  std::size_t size() const { return count_; }
  std::size_t capacity() const { return N; }
  bool empty() const { return count_ == 0; }
  bool full() const { return count_ == N; }

  T operator[](std::size_t index) const {
    std::size_t start = (count_ < N) ? 0 : write_index_;
    return data_[(start + index) % N];
  }

  T newest() const {
    if (count_ == 0) return T{};
    return data_[(write_index_ + N - 1) % N];
  }

  T oldest() const {
    if (count_ == 0) return T{};
    std::size_t start = (count_ < N) ? 0 : write_index_;
    return data_[start];
  }

  T Sum() const {
    T sum{};
    for (std::size_t i = 0; i < count_; ++i) {
      sum += (*this)[i];
    }
    return sum;
  }

  T Average() const {
    if (count_ == 0) return T{};
    return Sum() / static_cast<T>(count_);
  }

  T Min() const {
    if (count_ == 0) return T{};
    T min_val = (*this)[0];
    for (std::size_t i = 1; i < count_; ++i) {
      if ((*this)[i] < min_val) min_val = (*this)[i];
    }
    return min_val;
  }

  T Max() const {
    if (count_ == 0) return T{};
    T max_val = (*this)[0];
    for (std::size_t i = 1; i < count_; ++i) {
      if ((*this)[i] > max_val) max_val = (*this)[i];
    }
    return max_val;
  }

  const std::array<T, N>& raw_data() const { return data_; }
  std::size_t write_index() const { return write_index_; }

 private:
  std::array<T, N> data_{};
  std::size_t write_index_{0};
  std::size_t count_{0};
};

}  // namespace engine::profiling

#endif  // ENGINE_PROFILING_RING_BUFFER_H_
