/**
 * @file sparse_array.hpp
 * @brief Efficient component storage using optional values
 * @author Enzo Gallini
 * @version 1.0.0
 *
 * @details
 * SparseArray provides a memory-efficient storage mechanism for components
 * using std::vector<std::optional<T>>. This design allows:
 * - O(1) access to components by entity index
 * - Automatic gaps (nullopt) for entities without a component
 * - Efficient iteration over active components
 * - Minimal memory overhead per component
 */

#ifndef ENGINE_ECS_SPARSE_ARRAY_H_
#define ENGINE_ECS_SPARSE_ARRAY_H_

#include <optional>
#include <vector>

namespace engine::ecs {

/**
 * @class SparseArray
 * @brief Generic sparse component storage container
 * @tparam Component The component type to store
 *
 * @details
 * SparseArray uses a vector of optional values to efficiently store components
 * for potentially many entities. This allows:
 *
 * - **Direct Access**: O(1) lookup via entity index
 * - **Memory Efficiency**: Only one bool overhead per component per entity
 * - **Sparse Storage**: Missing components are represented as std::nullopt
 * - **Iteration**: Easy filtering of active components
 *
 * Memory layout:
 * @code
 * Index   | Value
 * --------|----------
 * 0       | Position{10, 20}  (active)
 * 1       | nullopt            (inactive)
 * 2       | Position{30, 40}  (active)
 * @endcode
 *
 * @section usage Usage
 * @code
 * SparseArray<Position> positions;
 *
 * // Emplace component at index 0
 * positions.EmplaceAt(0, 10.0f, 20.0f);
 *
 * // Check if component exists
 * if (positions[0].has_value()) {
 *     float x = positions[0].value().x;
 * }
 *
 * // Erase component
 * positions.Erase(0);  // Sets to nullopt
 *
 * // Iterate over all slots
 * for (auto& opt_component : positions) {
 *     if (opt_component.has_value()) {
 *         // Process active component
 *     }
 * }
 * @endcode
 *
 * @section performance Performance
 * - Access: O(1)
 * - Insertion: O(n) if resize needed, O(1) otherwise
 * - Deletion: O(1)
 * - Memory: 1 byte overhead per entity per component type
 *
 * @see Zipper
 * @see Registry
 */
template <typename Component>
class SparseArray {
 public:
  /// @brief Value type: optional component
  using ValueType = std::optional<Component>;

  /// @brief Mutable reference to value
  using ReferenceType = ValueType&;

  /// @brief Const reference to value
  using ConstReferenceType = const ValueType&;

  /// @brief Internal container type
  using Container_t = std::vector<ValueType>;

  /// @brief Size type
  using SizeType = typename Container_t::size_type;

  /// @brief Mutable iterator
  using Iterator = typename Container_t::iterator;

  /// @brief Const iterator
  using ConstIterator = typename Container_t::const_iterator;

 public:
  /// @brief Default constructor
  SparseArray() = default;

  /// @brief Copy constructor
  SparseArray(const SparseArray&) = default;

  /// @brief Move constructor
  SparseArray(SparseArray&&) noexcept = default;

  /// @brief Destructor
  ~SparseArray() = default;

  /// @brief Copy assignment operator
  SparseArray& operator=(const SparseArray&) = default;

  /// @brief Move assignment operator
  SparseArray& operator=(SparseArray&&) noexcept = default;

  /**
   * @brief Access component by index
   * @param idx Entity index
   * @return Mutable reference to optional component
   *
   * @note
   * Does not perform bounds checking. Accessing an index >= size()
   * results in undefined behavior.
   */
  ReferenceType operator[](size_t idx) { return data_[idx]; }

  /**
   * @brief Access component by index (const)
   * @param idx Entity index
   * @return Const reference to optional component
   */
  ConstReferenceType operator[](size_t idx) const { return data_[idx]; }

  /**
   * @brief Begin iterator (mutable)
   * @return Iterator to first element
   */
  Iterator begin() { return data_.begin(); }

  /**
   * @brief Begin iterator (const)
   * @return Const iterator to first element
   */
  ConstIterator begin() const { return data_.begin(); }

  /**
   * @brief Begin iterator (const)
   * @return Const iterator to first element
   */
  ConstIterator cbegin() const { return data_.cbegin(); }

  /**
   * @brief End iterator (mutable)
   * @return Iterator to one past last element
   */
  Iterator end() { return data_.end(); }

  /**
   * @brief End iterator (const)
   * @return Const iterator to one past last element
   */
  ConstIterator end() const { return data_.end(); }

  /**
   * @brief End iterator (const)
   * @return Const iterator to one past last element
   */
  ConstIterator cend() const { return data_.cend(); }

  /**
   * @brief Get number of slots in array
   * @return Size of internal vector
   *
   * @note
   * This is the total number of slots, including both active and inactive
   * (nullopt) components.
   */
  SizeType size() const { return data_.size(); }

  /**
   * @brief Insert component by copy at position
   * @param pos Entity index where component should be inserted
   * @param comp Component to insert (copied)
   * @return Reference to inserted component (wrapped in optional)
   *
   * @details
   * If pos >= size(), automatically resizes the array to accommodate the new
   * index. The new array slots are filled with nullopt.
   *
   * @example
   * @code
   * Position pos(10.0f, 20.0f);
   * array.InsertAt(5, pos);  // Inserts copy of pos at index 5
   * @endcode
   */
  ReferenceType InsertAt(SizeType pos, const Component& comp) {
    if (pos >= data_.size()) data_.resize(pos + 1);
    data_[pos] = comp;
    return data_[pos];
  }

  /**
   * @brief Insert component by move at position
   * @param pos Entity index where component should be inserted
   * @param comp Component to insert (moved)
   * @return Reference to inserted component (wrapped in optional)
   *
   * @details
   * If pos >= size(), automatically resizes the array to accommodate the new
   * index.
   */
  ReferenceType InsertAt(SizeType pos, Component&& comp) {
    if (pos >= data_.size()) data_.resize(pos + 1);
    data_[pos] = std::move(comp);
    return data_[pos];
  }

  /**
   * @brief Emplace component in-place at position
   * @tparam Params Types of constructor parameters
   * @param pos Entity index where component should be emplaced
   * @param params Constructor arguments for component
   * @return Reference to emplaced component (wrapped in optional)
   *
   * @details
   * Constructs the component in-place using the provided arguments.
   * If pos >= size(), automatically resizes the array.
   *
   * @example
   * @code
   * array.EmplaceAt(3, 15.0f, 25.0f);  // Constructs Position(15, 25)
   * @endcode
   */
  template <class... Params>
  ReferenceType EmplaceAt(SizeType pos, Params&&... params) {
    if (pos >= data_.size()) data_.resize(pos + 1);
    data_[pos].emplace(std::forward<Params>(params)...);
    return data_[pos];
  }

  /**
   * @brief Erase component at position
   * @param pos Entity index to erase
   *
   * @details
   * Sets the optional at the given position to nullopt, effectively
   * removing the component from the entity. Safe to call on non-existent
   * indices (does nothing if pos >= size()).
   */
  void Erase(SizeType pos) {
    if (pos < data_.size()) data_[pos] = std::nullopt;
  }

  /**
   * @brief Get index of a reference within the array
   * @param val Reference to optional component
   * @return Index of the component in the array
   *
   * @warning
   * This function performs pointer arithmetic and does NOT verify that
   * the reference actually belongs to this array. Using an invalid
   * reference results in undefined behavior.
   */
  SizeType GetIndex(const ValueType& val) const {
    return std::distance(data_.data(), std::addressof(val));
  }

 private:
  /// @brief Internal container
  Container_t data_;
};

}  // namespace engine::ecs

#endif /* !ENGINE_ECS_SPARSE_ARRAY_H_ */