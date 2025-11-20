#ifndef ENGINE_ECS_INDEXED_ZIPPER_H_
#define ENGINE_ECS_INDEXED_ZIPPER_H_

#include <iterator>
#include <tuple>
#include <utility>

namespace engine::ecs {

template <class... Containers>
class IndexedZipper;

template <class... Containers>
class IndexedZipperIterator {
  template <class Container>
  using IteratorT = decltype(std::declval<Container>().begin());

  template <class Container>
  using ItReferenceT = typename IteratorT<Container>::reference;

 public:
  using ValueType =
      std::tuple<size_t, decltype(std::declval<ItReferenceT<Containers>>())...>;
  using Reference = ValueType;
  using Pointer = void;
  using DifferenceType = size_t;
  using IteratorCategory = std::forward_iterator_tag;
  using IteratorTuple = std::tuple<IteratorT<Containers>...>;

  friend class IndexedZipper<Containers...>;

  IndexedZipperIterator(const IteratorTuple& it_tuple, size_t max,
                        size_t idx = 0)
      : current_(it_tuple), max_(max), idx_(idx) {
    if (idx_ < max_ && !AllSet(seq_)) IncrAll(seq_);
  }

  IndexedZipperIterator(const IndexedZipperIterator& z)
      : current_(z.current_), max_(z.max_), idx_(z.idx_) {}

  IndexedZipperIterator& operator++() {
    IncrAll(seq_);
    return *this;
  }

  IndexedZipperIterator operator++(int) {
    IndexedZipperIterator tmp = *this;
    IncrAll(seq_);
    return tmp;
  }

  ValueType operator*() { return ToValue(seq_); }

  ValueType operator->() { return ToValue(seq_); }

  friend bool operator==(const IndexedZipperIterator& lhs,
                         const IndexedZipperIterator& rhs) {
    return lhs.idx_ == rhs.idx_;
  }

  friend bool operator!=(const IndexedZipperIterator& lhs,
                         const IndexedZipperIterator& rhs) {
    return lhs.idx_ != rhs.idx_;
  }

 private:
  template <size_t... Is>
  void IncrAll(std::index_sequence<Is...>) {
    (++std::get<Is>(current_), ...);
    idx_++;
    while (idx_ < max_ && !AllSet(seq_)) {
      (++std::get<Is>(current_), ...);
      idx_++;
    }
  }

  template <size_t... Is>
  bool AllSet(std::index_sequence<Is...>) {
    return (std::get<Is>(current_)->has_value() && ...);
  }

  template <size_t... Is>
  ValueType ToValue(std::index_sequence<Is...>) {
    return std::tuple_cat(std::make_tuple(idx_),
                          std::tie(*std::get<Is>(current_)...));
  }

  IteratorTuple current_;
  size_t max_;
  size_t idx_;
  static constexpr std::index_sequence_for<Containers...> seq_{};
};

template <class... Containers>
class IndexedZipper {
 public:
  using Iterator = IndexedZipperIterator<Containers...>;
  using IteratorTuple = typename Iterator::IteratorTuple;

  IndexedZipper(Containers&... cs) {
    begin_ = std::make_tuple(cs.begin()...);
    end_ = ComputeEnd(cs...);
    size_ = ComputeSize(cs...);
  }

  Iterator begin() { return Iterator(begin_, size_); }

  Iterator end() { return Iterator(begin_, size_, size_); }

 private:
  static size_t ComputeSize(Containers&... containers) {
    return std::min({containers.size()...});
  }

  static IteratorTuple ComputeEnd(Containers&... containers) {
    return std::make_tuple(containers.end()...);
  }

  IteratorTuple begin_;
  IteratorTuple end_;
  size_t size_;
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_INDEXED_ZIPPER_H_