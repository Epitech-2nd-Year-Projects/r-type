/**
 * @file registry.hpp
 * @brief Central ECS component and system management
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
#include "system.h"
#include "system_scheduler.h"

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

  explicit Registry(time::TimeDelta fixed_timestep =
                        time::TimeDelta::from_seconds(1.0f / 60.0f));

  ~Registry();

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

  /**
   * @brief Register a lambda-based system
   * @tparam Components Component types required by the system
   * @tparam Function System function type
   * @tparam ExtraArgs Additional argument types
   * @param f System function/callable
   * @param type System type (Fixed or Variable)
   * @param priority Execution priority (default: 100)
   * @param extra_args Additional arguments to pass to system
   *
   * @example
   * @code
   * // Variable timestep (every frame)
   * registry.add_system<Position, Velocity>(
   *     [](Registry& reg, SparseArray<Position>& pos,
   *        SparseArray<Velocity>& vel) {
   *         for (auto &&[p, v] : Zipper(pos, vel)) {
   *             p.value().x += v.value().vx;
   *         }
   *     },
   *     SystemType::Variable,
   *     kDefaultPriority
   * );
   *
   * // Fixed timestep (physics)
   * registry.add_system<Position, Velocity>(
   *     [](Registry& reg, SparseArray<Position>& pos,
   *        SparseArray<Velocity>& vel, float gravity) {
   *         for (auto &&[p, v] : Zipper(pos, vel)) {
   *             v.value().vy += gravity;
   *             p.value().x += v.value().vx;
   *             p.value().y += v.value().vy;
   *         }
   *     },
   *     SystemType::Fixed,
   *     kHighPriority,
   *     9.81f  // gravity extra arg
   * );
   * @endcode
   */
  template <class... Components, typename Function, typename... ExtraArgs>
  void AddSystem(Function&& f, SystemType type = SystemType::Variable,
                 SystemPriority priority = kDefaultPriority,
                 ExtraArgs&&... extra_args) {
    auto system_wrapper = [f = std::forward<Function>(f),
                           extra = std::make_tuple(
                               std::forward<ExtraArgs>(extra_args)...)](
                              Registry& reg, time::TimeDelta dt) {
      auto comp_arrays =
          std::make_tuple(std::ref(reg.GetComponents<Components>())...);

      std::apply(
          [&](auto&... arrays) {
            std::apply(
                [&](auto&&... extra_vals) {
                  if constexpr (std::is_invocable_v<
                                    Function, Registry&, decltype(arrays)...,
                                    time::TimeDelta, decltype(extra_vals)...>) {
                    f(reg, arrays..., dt, extra_vals...);
                  } else {
                    f(reg, arrays..., extra_vals...);
                  }
                },
                extra);
          },
          comp_arrays);
    };

    scheduler_->RegisterSystem(std::move(system_wrapper), type, priority);
  }

  /**
   * @brief Register an object-oriented system
   * @param system Unique pointer to system object
   * @param type System type (Fixed or Variable)
   * @param priority Execution priority
   *
   * @example
   * @code
   * class PhysicsSystem : public ISystem {
   *   void fixed_update(Registry& reg, TimeDelta dt) override {
   *     // ...
   *   }
   * };
   *
   * registry.add_system(
   *   std::make_unique<PhysicsSystem>(),
   *   SystemType::Fixed,
   *   500
   * );
   * @endcode
   */
  void AddSystemClass(std::shared_ptr<ISystem> system,
                      SystemType type = SystemType::Variable,
                      SystemPriority priority = kDefaultPriority) {
    owned_systems_.push_back(system);

    scheduler_->RegisterSystem(
        [system](Registry& reg, time::TimeDelta dt) {
          system->Update(reg, dt);
        },
        type, priority);
  }

  /**
   * @brief Register a named object-oriented system
   * @param name Unique name for the system (for hot-reloading/retrieval)
   * @param system Unique pointer to system object
   * @param type System type (Fixed or Variable)
   * @param priority Execution priority
   */
  void AddSystemClass(const std::string& name, std::shared_ptr<ISystem> system,
                      SystemType type = SystemType::Variable,
                      SystemPriority priority = kDefaultPriority) {
    if (named_systems_.find(name) != named_systems_.end()) {
      throw std::runtime_error("System with name '" + name +
                               "' already exists. Use GetSystem to update it.");
    }
    named_systems_[name] = system;
    AddSystemClass(system, type, priority);
  }

  /**
   * @brief Retrieve a registered system by name.
   * @param name The system name.
   * @return The system pointer or nullptr if not found.
   */
  std::shared_ptr<ISystem> GetSystem(const std::string& name) {
    auto it = named_systems_.find(name);
    if (it != named_systems_.end()) {
      return it->second;
    }
    return nullptr;
  }

  /**
   * @brief Execute all registered systems
   * @param dt Time since last frame
   *
   * @details
   * - Runs variable systems once with actual dt
   * - Accumulates time for fixed systems
   * - Executes fixed systems 0-N times per frame
   *
   * @example
   * @code
   * Clock clock;
   * while (game_running) {
   *   TimeDelta dt = clock.restart();
   *   registry.update_systems(dt);
   * }
   * @endcode
   */
  void UpdateSystems(time::TimeDelta dt);

  /**
   * @brief Set fixed timestep interval
   * @param timestep New fixed timestep
   */
  void SetFixedTimestep(time::TimeDelta timestep);

  /**
   * @brief Get current fixed timestep
   * @return Fixed timestep duration
   */
  time::TimeDelta FixedTimestep() const;

  /**
   * @brief Clear all registered systems
   */
  void ClearSystems();

 private:
  /// @brief Component storage map (type_index -> SparseArray<Component>)
  std::unordered_map<std::type_index, std::any> components_arrays_;

  /// @brief Component deletion functions for each type
  std::vector<std::function<void(Registry&, const EntityId&)>>
      component_deleters_;

  /// @brief Next entity ID to assign
  std::size_t next_entity_id_ = 0;

  /// @brief System scheduler
  std::unique_ptr<SystemScheduler> scheduler_;

  /// @brief OOP-style systems storage
  std::vector<std::shared_ptr<ISystem>> owned_systems_;

  /// @brief Named systems for retrieval (e.g. hot-reloading)
  std::unordered_map<std::string, std::shared_ptr<ISystem>> named_systems_;
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_REGISTRY_H_