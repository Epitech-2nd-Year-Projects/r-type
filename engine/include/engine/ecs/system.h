/**
 * @file system.h
 * @brief ECS System interfaces and scheduling types
 * @version 1.0.0
 *
 * @details
 * Defines the base interfaces and types for ECS systems, including
 * scheduling policies (fixed vs. variable timestep) and priorities.
 */

#ifndef ENGINE_ECS_SYSTEM_H_
#define ENGINE_ECS_SYSTEM_H_

#include <cstdint>

#include "engine/time/time_delta.h"

namespace engine::ecs {

// Forward declaration
class Registry;

/**
 * @enum SystemType
 * @brief Execution policy for systems
 *
 * @details
 * - **Variable**: Runs every frame with actual delta time (rendering, input)
 * - **Fixed**: Runs at fixed intervals regardless of framerate (physics, AI)
 *
 * @example
 * @code
 * // Rendering runs every frame
 * registry.add_system<Position, Sprite>(render_system, SystemType::Variable);
 *
 * // Physics runs at fixed 60Hz
 * registry.add_system<Position, Velocity>(physics_system, SystemType::Fixed);
 * @endcode
 */
enum class SystemType : std::uint8_t {
  /// @brief Runs every frame with variable delta time
  Variable = 0,

  /// @brief Runs at fixed intervals (accumulates time)
  Fixed = 1
};

/**
 * @typedef SystemPriority
 * @brief Execution order priority (higher = runs earlier)
 *
 * @details
 * Systems with higher priority execute first. Useful for enforcing
 * execution order within the same SystemType.
 *
 * Default priorities:
 * - Input handling: 1000
 * - Physics: 500
 * - Game logic: 100
 * - Rendering: 0
 *
 * @example
 * @code
 * constexpr SystemPriority kInputPriority = 1000;
 * constexpr SystemPriority kPhysicsPriority = 500;
 *
 * registry.add_system<...>(input_system, SystemType::Variable, kInputPriority);
 * registry.add_system<...>(physics_system, SystemType::Fixed,
 * kPhysicsPriority);
 * @endcode
 */
using SystemPriority = std::int32_t;

/// @brief Default priority for systems
constexpr SystemPriority kDefaultPriority = 100;

/// @brief High priority (input, critical systems)
constexpr SystemPriority kHighPriority = 1000;

/// @brief Low priority (rendering, non-critical)
constexpr SystemPriority kLowPriority = 0;

/**
 * @class ISystem
 * @brief Abstract base interface for object-oriented systems
 *
 * @details
 * Optional base class for systems that prefer OOP style over lambdas.
 * Implement either `update()` for variable timestep or `fixed_update()`
 * for fixed timestep.
 *
 * @note
 * Most users will use lambda-based systems via Registry::add_system<>().
 * This interface is provided for complex systems that benefit from
 * encapsulation and state.
 *
 * @section usage Usage
 * @code
 * class PhysicsSystem : public ISystem {
 *  public:
 *   void fixed_update(Registry& registry, time::TimeDelta dt) override {
 *     auto& positions = registry.get_components<Position>();
 *     auto& velocities = registry.get_components<Velocity>();
 *
 *     for (auto &&[pos, vel] : Zipper(positions, velocities)) {
 *       pos.value().x += vel.value().vx * dt.as_seconds();
 *       pos.value().y += vel.value().vy * dt.as_seconds();
 *     }
 *   }
 * };
 *
 * // Register
 * registry.add_system(
 *   std::make_unique<PhysicsSystem>(),
 *   SystemType::Fixed,
 *   500
 * );
 * @endcode
 */
class ISystem {
 public:
  virtual ~ISystem() = default;

  /**
   * @brief Update called every frame (variable timestep)
   * @param registry Reference to ECS registry
   * @param dt Time since last frame
   *
   * Override this for systems that should run every frame.
   */
  virtual void Update(Registry& registry, time::TimeDelta dt) = 0;
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_SYSTEM_H_