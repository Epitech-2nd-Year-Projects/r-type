/**
 * @file entity_id.hpp
 * @brief Type-safe Entity Identification
 * @author Enzo Gallini
 * @version 1.0.0
 *
 * @details
 * EntityId provides a type-safe wrapper around size_t to represent entity
 * identifiers. This prevents accidental mixing of entity IDs with other numeric
 * values and enables the use of EntityId in containers like std::map and
 * std::set.
 */

#ifndef ENGINE_ECS_ENTITY_ID_H_
#define ENGINE_ECS_ENTITY_ID_H_

#include <cstddef>
#include <functional>

namespace engine::ecs {

/**
 * @class EntityId
 * @brief Type-safe identifier for entities in the ECS system
 *
 * @details
 * EntityId is a lightweight wrapper around size_t that provides:
 * - Type safety: Prevents accidental use of regular integers as entity IDs
 * - Comparison operators: ==, !=, <
 * - Implicit conversion to size_t for indexing
 * - Hash support for use in unordered containers
 *
 * @note EntityId instances should only be created through
 * Registry::SpawnEntity() or Registry::EntityFromIndex(). Manual construction
 * is not recommended.
 *
 * @example
 * @code
 * EntityId entity1 = registry.SpawnEntity();
 * EntityId entity2 = registry.SpawnEntity();
 *
 * if (entity1 == entity2) {
 *     // Same entity
 * }
 *
 * size_t index = static_cast<size_t>(entity1);  // Convert to index
 * @endcode
 *
 * @see Registry::SpawnEntity()
 * @see Registry::EntityFromIndex()
 */
class EntityId {
 private:
  /// @brief Internal entity identifier
  std::size_t id_;

  /// @brief Registry is friend to create EntityId instances
  friend class Registry;

  /**
   * @brief Private constructor for Registry
   * @param id The internal entity identifier
   */
  explicit EntityId(std::size_t id) noexcept : id_(id) {}

 public:
  /**
   * @brief Convert EntityId to its underlying size_t index
   * @return The internal entity identifier as size_t
   *
   * @details
   * This implicit conversion allows EntityId to be used for array indexing
   * and other operations requiring a numeric value.
   *
   * @example
   * @code
   * EntityId e = registry.SpawnEntity();
   * size_t index = static_cast<size_t>(e);
   * @endcode
   */
  operator std::size_t() const noexcept { return id_; }

  /**
   * @brief Equality comparison operator
   * @param other EntityId to compare with
   * @return true if both EntityIds represent the same entity
   */
  bool operator==(const EntityId& other) const noexcept {
    return id_ == other.id_;
  }

  /**
   * @brief Inequality comparison operator
   * @param other EntityId to compare with
   * @return true if EntityIds represent different entities
   */
  bool operator!=(const EntityId& other) const noexcept {
    return id_ != other.id_;
  }

  /**
   * @brief Less-than comparison operator
   * @param other EntityId to compare with
   * @return true if this EntityId is less than other
   *
   * @details
   * This operator enables EntityId to be used in sorted containers
   * like std::map and std::set.
   */
  bool operator<(const EntityId& other) const noexcept {
    return id_ < other.id_;
  }
};

}  // namespace engine::ecs

/// @brief Hash specialization for EntityId
/// @details Enables EntityId to be used as key in std::unordered_map
template <>
struct std::hash<engine::ecs::EntityId> {
  /**
   * @brief Hash function for EntityId
   * @param e The EntityId to hash
   * @return Hash value for the EntityId
   */
  std::size_t operator()(const engine::ecs::EntityId& e) const noexcept {
    return std::hash<std::size_t>()(static_cast<std::size_t>(e));
  }
};

#endif /* !ENGINE_ECS_ENTITY_ID_H_ */