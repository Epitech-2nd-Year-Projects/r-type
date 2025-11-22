/**
 * @file zipper.hpp
 * @brief Multi-component iteration without entity indices
 * @version 1.0.0
 *
 * @details
 * Zipper provides efficient iteration over entities that have specific
 * combinations of components. It automatically filters out entities
 * that don't have all required components.
 */

#ifndef ENGINE_ECS_ZIPPER_H_
#define ENGINE_ECS_ZIPPER_H_

#include <iterator>
#include <tuple>
#include <utility>

namespace engine::ecs {

template <class... Containers>
class Zipper;

/**
 * @class ZipperIterator
 * @brief Iterator for zipped container traversal
 * @tparam Containers Container types to zip
 *
 * @details
 * ZipperIterator iterates over multiple containers simultaneously, yielding
 * values only when all containers have a value at the current position.
 *
 * This enables efficient component filtering: if an entity is missing any
 * required component, it's skipped automatically.
 *
 * @see Zipper
 */
template <class... Containers>
class ZipperIterator {
  template <class Container>
  using IteratorT = decltype(std::declval<Container>().begin());

  template <class Container>
  using ItReferenceT = typename IteratorT<Container>::reference;

 public:
  /// @brief Value type: tuple of references
  using ValueType =
      std::tuple<decltype(std::declval<ItReferenceT<Containers>>())...>;

  /// @brief Reference type
  using Reference = ValueType;

  /// @brief Pointer type (void for tuple results)
  using Pointer = void;

  /// @brief Difference type
  using DifferenceType = size_t;

  /// @brief Iterator category
  using IteratorCategory = std::forward_iterator_tag;

  /// @brief Tuple of iterators
  using IteratorTuple = std::tuple<IteratorT<Containers>...>;

  friend class Zipper<Containers...>;

  /**
   * @brief Construct zipper iterator
   * @param it_tuple Tuple of iterators to underlying containers
   * @param max Maximum index to iterate to
   * @param idx Current iteration index (default 0)
   */
  ZipperIterator(const IteratorTuple& it_tuple, size_t max, size_t idx = 0)
      : current_(it_tuple), max_(max), idx_(idx) {
    if (idx_ < max_ && !AllSet(seq_)) IncrAll(seq_);
  }

  /// @brief Copy constructor
  ZipperIterator(const ZipperIterator& z)
      : current_(z.current_), max_(z.max_), idx_(z.idx_) {}

  /**
   * @brief Pre-increment operator
   * @return Reference to this iterator
   */
  ZipperIterator& operator++() {
    IncrAll(seq_);
    return *this;
  }

  /**
   * @brief Post-increment operator
   * @return Copy of iterator before increment
   */
  ZipperIterator operator++(int) {
    ZipperIterator tmp = *this;
    IncrAll(seq_);
    return tmp;
  }

  /**
   * @brief Dereference operator
   * @return Tuple of references to current values
   */
  ValueType operator*() { return ToValue(seq_); }

  /**
   * @brief Member access operator
   * @return Tuple of references to current values
   */
  ValueType operator->() { return ToValue(seq_); }

  /**
   * @brief Equality comparison
   * @param lhs Left iterator
   * @param rhs Right iterator
   * @return true if both point to same position
   */
  friend bool operator==(const ZipperIterator& lhs, const ZipperIterator& rhs) {
    return lhs.idx_ == rhs.idx_;
  }

  /**
   * @brief Inequality comparison
   * @param lhs Left iterator
   * @param rhs Right iterator
   * @return true if iterators point to different positions
   */
  friend bool operator!=(const ZipperIterator& lhs, const ZipperIterator& rhs) {
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
    return std::tie(*std::get<Is>(current_)...);
  }

  IteratorTuple current_;
  size_t max_;
  size_t idx_;
  static constexpr std::index_sequence_for<Containers...> seq_{};
};

/**
 * @class Zipper
 * @brief Multi-container simultaneous iteration
 * @tparam Containers Container types to zip
 *
 * @details
 * Zipper enables elegant simultaneous iteration over multiple containers,
 * automatically filtering out indices where any container has a nullopt value.
 *
 * This is the primary mechanism for iterating over entities with specific
 * component combinations in the ECS system.
 *
 * @section usage Usage Example
 * @code
 * auto& positions = registry.GetComponents<Position>();
 * auto& velocities = registry.GetComponents<Velocity>();
 *
 * for (auto &&[pos, vel] : Zipper(positions, velocities)) {
 *     pos.value().x += vel.value().vx;
 *     pos.value().y += vel.value().vy;
 * }
 * @endcode
 *
 * @section performance Performance
 * - Iteration: O(n) where n = max container size
 * - Only visits indices where all containers have values
 * - Zero-copy: returns references to original data
 *
 * @see ZipperIterator
 * @see IndexedZipper
 */
template <class... Containers>
class Zipper {
 public:
  /// @brief Iterator type
  using Iterator = ZipperIterator<Containers...>;

  /// @brief Iterator tuple type
  using IteratorTuple = typename Iterator::IteratorTuple;

  /**
   * @brief Construct zipper from containers
   * @param cs Containers to zip
   */
  Zipper(Containers&... cs) {
    begin_ = std::make_tuple(cs.begin()...);
    end_ = ComputeEnd(cs...);
    size_ = ComputeSize(cs...);
  }

  /**
   * @brief Get begin iterator
   * @return Iterator to first valid combination
   */
  Iterator begin() { return Iterator(begin_, size_); }

  /**
   * @brief Get end iterator
   * @return Iterator to end position
   */
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

#endif  // ENGINE_ECS_ZIPPER_H_