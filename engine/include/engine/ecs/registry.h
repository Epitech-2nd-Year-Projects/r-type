/**
 * @file registry.hpp
 * @brief Central ECS component and system management
 * @author Enzo Gallini
 * @version 1.0.0
 *
 * @details
 * Registry is the central hub of the ECS system, managing:
 * - Component type registration
 * - Entity lifecycle (spawn/kill)
 * - Component attachment/detachment
 * - System registration and execution
 */

#ifndef ENGINE_ECS_REGISTRY_H_
#define ENGINE_ECS_REGISTRY_H_

#include <any>
#include <functional>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "entity_id.h"
#include "sparse_array.h"

namespace engine::ecs {

/**
 * @class Registry
 * @brief Central component and entity manager
 *
 * @details
 * Registry is the main interface for working with the ECS system. It provides:
 *
 * - **Component Registration**: Register component types before use
 * - **Entity Management**: Spawn and destroy entities
 * - **Component Lifecycle**: Attach, detach, and retrieve components
 * - **System Management**: Register and execute systems
 *
 * Registry uses type erasure (std::any) to store heterogeneous component
 * arrays.
 *
 * @section usage Usage Example
 * @code
 * Registry registry;
 *
 * // Register components
 * registry.RegisterComponent<Position>();
 * registry.RegisterComponent<Velocity>();
 * registry.RegisterComponent<Health>();
 *
 * // Create entity
 * EntityId player = registry.SpawnEntity();
 *
 * // Attach components
 * registry.EmplaceComponent<Position>(player, 0.0f, 0.0f);
 * registry.EmplaceComponent<Velocity>(player, 1.0f, 1.5f);
 * registry.EmplaceComponent<Health>(player, 100);
 *
 * // Remove component
 * registry.RemoveComponent<Health>(player);
 *
 * // Kill entity
 * registry.KillEntity(player);
 *
 * // Register system
 * registry.AddSystem<Position, Velocity>(
 *     [](Registry& reg, SparseArray<Position>& positions,
 *        SparseArray<Velocity>& velocities) {
 *         // System implementation
 *     });
 *
 * // Execute all systems
 * registry.RunSystems();
 * @endcode
 *
 * @section threading Thread Safety
 * Registry is NOT thread-safe. All operations must occur on the same thread.
 *
 * @see EntityId
 * @see SparseArray
 * @see Zipper
 */
class Registry {
 public:
  /**
   * @brief Register a component type
   * @tparam Component The component type to register
   * @return Reference to the SparseArray for this component
   *
   * @details
   * Must be called before using components of this type. Multiple calls
   * with the same type are safe (no-op after first call).
   *
   * @throws std::bad_any_cast if component already registered with wrong type
   *
   * @example
   * @code
   * registry.RegisterComponent<Position>();
   * registry.RegisterComponent<Velocity>();
   * @endcode
   */
  template <class Component>
  SparseArray<Component>& RegisterComponent() {
    auto type_idx = std::type_index(typeid(Component));

    if (components_arrays_.find(type_idx) == components_arrays_.end()) {
      components_arrays_[type_idx] = SparseArray<Component>();
      component_deleters_.push_back([](Registry& reg, const EntityId& e) {
        auto& components = reg.GetComponents<Component>();
        components.Erase(static_cast<std::size_t>(e));
      });
    }
    return std::any_cast<SparseArray<Component>&>(components_arrays_[type_idx]);
  }

  /**
   * @brief Get mutable access to component storage
   * @tparam Component The component type
   * @return Reference to SparseArray for this component
   *
   * @throws std::runtime_error if component type not registered
   *
   * @example
   * @code
   * auto& positions = registry.GetComponents<Position>();
   * @endcode
   */
  template <class Component>
  SparseArray<Component>& GetComponents() {
    auto type_idx = std::type_index(typeid(Component));
    auto it = components_arrays_.find(type_idx);

    if (it == components_arrays_.end())
      throw std::runtime_error("Component type not registered");
    return std::any_cast<SparseArray<Component>&>(components_arrays_[type_idx]);
  }

  /**
   * @brief Get const access to component storage
   * @tparam Component The component type
   * @return Const reference to SparseArray for this component
   *
   * @throws std::runtime_error if component type not registered
   */
  template <class Component>
  const SparseArray<Component>& GetComponents() const {
    auto type_idx = std::type_index(typeid(Component));
    auto it = components_arrays_.find(type_idx);

    if (it == components_arrays_.end())
      throw std::runtime_error("Component type not registered");
    return std::any_cast<const SparseArray<Component>&>(it->second);
  }

  /**
   * @brief Create a new entity
   * @return EntityId of the newly created entity
   *
   * @details
   * Creates a new entity with a unique ID. The entity initially has no
   * components. Entity IDs are assigned sequentially starting from 0.
   *
   * @example
   * @code
   * EntityId player = registry.SpawnEntity();
   * EntityId enemy = registry.SpawnEntity();
   * @endcode
   */
  EntityId SpawnEntity() noexcept { return EntityId(next_entity_id_++); }

  /**
   * @brief Create EntityId from an index
   * @param idx The entity index
   * @return EntityId representing the given index
   *
   * @details
   * Utility function to construct an EntityId from a size_t index.
   * Primarily used for internal operations.
   */
  EntityId EntityFromIndex(std::size_t idx) noexcept { return EntityId(idx); }

  /**
   * @brief Destroy an entity and all its components
   * @param e EntityId to destroy
   *
   * @details
   * Removes all components associated with the entity by setting them to
   * nullopt. The entity ID becomes invalid after this call. Calling this
   * multiple times with the same entity is safe (subsequent calls do nothing).
   *
   * @example
   * @code
   * registry.KillEntity(player);
   * @endcode
   */
  void KillEntity(const EntityId& e) {
    for (auto& deleter : component_deleters_) {
      deleter(*this, e);
    }
  }

  /**
   * @brief Add component by move semantics
   * @tparam Component Component type to add
   * @param to Target entity
   * @param component Component instance (moved)
   * @return Reference to the added component
   *
   * @throws std::runtime_error if component type not registered
   *
   * @example
   * @code
   * Position pos(10.0f, 20.0f);
   * registry.AddComponent<Position>(player, std::move(pos));
   * @endcode
   */
  template <typename Component>
  typename SparseArray<Component>::ReferenceType AddComponent(
      const EntityId& to, Component&& component) {
    auto& components = GetComponents<Component>();
    return components.InsertAt(static_cast<std::size_t>(to),
                               std::forward<Component>(component));
  }

  /**
   * @brief Add component by copy semantics
   * @tparam Component Component type to add
   * @param to Target entity
   * @param component Component instance (copied)
   * @return Reference to the added component
   *
   * @throws std::runtime_error if component type not registered
   *
   * @example
   * @code
   * Position pos(10.0f, 20.0f);
   * registry.AddComponent<Position>(player, pos);  // Copied
   * @endcode
   */
  template <typename Component>
  typename SparseArray<Component>::ReferenceType AddComponent(
      const EntityId& to, const Component& component) {
    auto& components = GetComponents<Component>();
    return components.InsertAt(static_cast<std::size_t>(to), component);
  }

  /**
   * @brief Emplace component in-place
   * @tparam Component Component type to emplace
   * @tparam Params Constructor parameter types
   * @param to Target entity
   * @param params Constructor arguments
   * @return Reference to the emplaced component
   *
   * @throws std::runtime_error if component type not registered
   *
   * @example
   * @code
   * registry.EmplaceComponent<Position>(player, 10.0f, 20.0f);
   * @endcode
   */
  template <typename Component, typename... Params>
  typename SparseArray<Component>::ReferenceType EmplaceComponent(
      const EntityId& to, Params&&... params) {
    auto& components = GetComponents<Component>();
    return components.EmplaceAt(static_cast<std::size_t>(to),
                                std::forward<Params>(params)...);
  }

  /**
   * @brief Remove component from entity
   * @tparam Component Component type to remove
   * @param from Target entity
   *
   * @throws std::runtime_error if component type not registered
   *
   * @details
   * Sets the component at the entity's index to nullopt. Safe to call on
   * entities without the component.
   *
   * @example
   * @code
   * registry.RemoveComponent<Health>(player);
   * @endcode
   */
  template <typename Component>
  void RemoveComponent(const EntityId& from) {
    auto& components = GetComponents<Component>();
    components.Erase(static_cast<std::size_t>(from));
  }

  /**
   * @brief Register a system
   * @tparam Components Component types required by the system
   * @tparam Function System function type
   * @tparam ExtraArgs Additional argument types
   * @param f System function/callable
   * @param extra_args Additional arguments to pass to system
   *
   * @details
   * Registers a system that will be executed by RunSystems(). The system
   * function receives the Registry and SparseArray references for each
   * specified component type.
   *
   * @example
   * @code
   * registry.AddSystem<Position, Velocity>(
   *     [](Registry& reg, SparseArray<Position>& pos,
   *        SparseArray<Velocity>& vel) {
   *         for (auto &&[p, v] : Zipper(pos, vel)) {
   *             p.value().x += v.value().vx;
   *         }
   *     });
   *
   * // With extra arguments
   * registry.AddSystem<Health, Damage>(
   *     [](Registry& reg, SparseArray<Health>& hp,
   *        SparseArray<Damage>& dmg, float multiplier) {
   *         for (auto &&[h, d] : Zipper(hp, dmg)) {
   *             h.value().hp -= d.value().value * multiplier;
   *         }
   *     }, 1.5f);
   * @endcode
   */
  template <class... Components, typename Function, typename... ExtraArgs>
  void AddSystem(Function&& f, ExtraArgs&&... extra_args) {
    auto system_wrapper = [f = std::forward<Function>(f),
                           extra = std::make_tuple(std::forward<ExtraArgs>(
                               extra_args)...)](Registry& reg) {
      auto comp_arrays =
          std::make_tuple(std::ref(reg.GetComponents<Components>())...);

      std::apply(
          [&](auto&... arrays) {
            std::apply(
                [&](auto&&... extra_vals) { f(reg, arrays..., extra_vals...); },
                extra);
          },
          comp_arrays);
    };
    systems_.push_back(system_wrapper);
  }

  /**
   * @brief Execute all registered systems
   *
   * @details
   * Executes all systems in the order they were registered. Each system
   * operates on entities that have all its required components.
   *
   * @example
   * @code
   * registry.RunSystems();  // Execute all registered systems
   * @endcode
   */
  void RunSystems() {
    for (auto& system : systems_) {
      system(*this);
    }
  }

 private:
  /// @brief Component storage map (type_index -> SparseArray<Component>)
  std::unordered_map<std::type_index, std::any> components_arrays_;

  /// @brief Component deletion functions for each type
  std::vector<std::function<void(Registry&, const EntityId&)>>
      component_deleters_;

  /// @brief Next entity ID to assign
  std::size_t next_entity_id_ = 0;

  /// @brief Registered systems
  std::vector<std::function<void(Registry&)>> systems_;
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_REGISTRY_H_