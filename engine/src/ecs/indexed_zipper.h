/**
 * @file indexed_zipper.hpp
 * @brief Multi-component iteration with entity indices
 * @author Enzo Gallini
 * @version 1.0.0
 *
 * @details
 * IndexedZipper extends Zipper by also providing the entity index
 * (array position) during iteration. This is useful for systems that
 * need to know which entity is being processed.
 */

#ifndef ENGINE_ECS_INDEXED_ZIPPER_H_
#define ENGINE_ECS_INDEXED_ZIPPER_H_

#include <iterator>
#include <tuple>
#include <utility>

namespace engine::ecs {

template <class... Containers>
class IndexedZipper;

/**
 * @class IndexedZipperIterator
 * @brief Iterator that includes entity indices
 * @tparam Containers Container types to iterate
 *
 * @details
 * Similar to ZipperIterator but also yields the current entity index
 * as the first element of the returned tuple.
 *
 * @see IndexedZipper
 */
template <class... Containers>
class IndexedZipperIterator {
  template <class Container>
  using IteratorT = decltype(std::declval<Container>().begin());

  template <class Container>
  using ItReferenceT = typename IteratorT<Container>::reference;

 public:
  /// @brief Value type: (index, values...)
  using ValueType =
      std::tuple<size_t, decltype(std::declval<ItReferenceT<Containers>>())...>;

  /// @brief Reference type
  using Reference = ValueType;

  /// @brief Pointer type
  using Pointer = void;

  /// @brief Difference type
  using DifferenceType = size_t;

  /// @brief Iterator category
  using IteratorCategory = std::forward_iterator_tag;

  /// @brief Tuple of iterators
  using IteratorTuple = std::tuple<IteratorT<Containers>...>;

  friend class IndexedZipper<Containers...>;

  /**
   * @brief Construct indexed zipper iterator
   * @param it_tuple Tuple of iterators
   * @param max Maximum index
   * @param idx Current index (default 0)
   */
  IndexedZipperIterator(const IteratorTuple& it_tuple, size_t max,
                        size_t idx = 0)
      : current_(it_tuple), max_(max), idx_(idx) {
    if (idx_ < max_ && !AllSet(seq_)) IncrAll(seq_);
  }

  /// @brief Copy constructor
  IndexedZipperIterator(const IndexedZipperIterator& z)
      : current_(z.current_), max_(z.max_), idx_(z.idx_) {}

  /**
   * @brief Pre-increment operator
   * @return Reference to this iterator
   */
  IndexedZipperIterator& operator++() {
    IncrAll(seq_);
    return *this;
  }

  /**
   * @brief Post-increment operator
   * @return Copy before increment
   */
  IndexedZipperIterator operator++(int) {
    IndexedZipperIterator tmp = *this;
    IncrAll(seq_);
    return tmp;
  }

  /**
   * @brief Dereference operator
   * @return Tuple (index, value1, value2, ...)
   */
  ValueType operator*() { return ToValue(seq_); }

  /**
   * @brief Member access operator
   * @return Tuple (index, value1, value2, ...)
   */
  ValueType operator->() { return ToValue(seq_); }

  /**
   * @brief Equality comparison
   * @param lhs Left iterator
   * @param rhs Right iterator
   * @return true if same position
   */
  friend bool operator==(const IndexedZipperIterator& lhs,
                         const IndexedZipperIterator& rhs) {
    return lhs.idx_ == rhs.idx_;
  }

  /**
   * @brief Inequality comparison
   * @param lhs Left iterator
   * @param rhs Right iterator
   * @return true if different positions
   */
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

/**
 * @class IndexedZipper
 * @brief Multi-container iteration with entity index tracking
 * @tparam Containers Container types to iterate
 *
 * @details
 * IndexedZipper combines the benefits of Zipper with entity index information.
 * It's particularly useful for:
 * - Logging/debugging (need to know which entity is being processed)
 * - Creating new entities with known indices
 * - Entity-specific operations
 *
 * @section usage Usage Example
 * @code
 * auto& positions = registry.GetComponents<Position>();
 * auto& velocities = registry.GetComponents<Velocity>();
 *
 * for (auto &&[idx, pos, vel] : IndexedZipper(positions, velocities)) {
 *     std::cout << "Entity " << idx << ": pos=(" << pos.value().x
 *               << ", " << pos.value().y << ")\n";
 * }
 * @endcode
 *
 * @see Zipper
 * @see IndexedZipperIterator
 */
template <class... Containers>
class IndexedZipper {
 public:
  /// @brief Iterator type
  using Iterator = IndexedZipperIterator<Containers...>;

  /// @brief Iterator tuple type
  using IteratorTuple = typename Iterator::IteratorTuple;

  /**
   * @brief Construct indexed zipper from containers
   * @param cs Containers to zip
   */
  IndexedZipper(Containers&... cs) {
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

#endif  // ENGINE_ECS_INDEXED_ZIPPER_H_