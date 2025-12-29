#ifndef GAME_LOGIC_SYSTEMS_COLLISION_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_COLLISION_SYSTEM_H_

#include <functional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/data_structures/spatial_grid.h"
#include "engine/ecs/entity_id.h"
#include "engine/ecs/system.h"
#include "engine/event.h"
#include "engine/math/rect.h"
#include "engine/scripting/script_engine.h"
#include "engine/time/time_delta.h"
#include "game_logic/components/damageable_component.h"

namespace game_logic::systems {

/**
 * @class CollisionSystem
 * @brief Handles AABB collision detection and resolution using spatial
 * partitioning.
 *
 * @details
 * Uses a fixed-size grid for spatial partitioning to reduce complexity from
 * O(N^2) to ~O(N). Detects and resolves collisions between:
 * - Player <-> Enemy
 * - PlayerProjectile <-> Enemy
 * - EnemyProjectile <-> Player
 */
/**
 * @brief Event emitted when two entities overlap
 */
struct EntityCollisionEvent {
  engine::ecs::EntityId entity_a;
  engine::ecs::EntityId entity_b;
};

class CollisionSystem : public engine::ecs::ISystem {
 public:
  static constexpr std::string_view kPlayerTag = "Player";
  static constexpr std::string_view kEnemyTag = "Enemy";

  CollisionSystem(engine::event::EventBus& event_bus,
                  engine::scripting::ScriptEngine& script_engine,
                  float cell_size = 100.0f);
  ~CollisionSystem() override = default;

  /**
   * @brief Update collisions. Rebuilds grid and checks collisions.
   */
  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;

 private:
  engine::event::EventBus& event_bus_;
  engine::scripting::ScriptEngine& script_engine_;
  engine::data_structures::SpatialGrid<engine::ecs::EntityId> grid_;

  /**
   * @brief Helper to resolve simple AABB interaction
   */
  bool CheckOneWayCollision(const engine::math::RectF& a,
                            const engine::math::RectF& b);

  /**
   * @brief Resolves collision between two generic entities.
   * Handles Player <-> Enemy crash damage.
   * @param registry The ECS registry.
   * @param e1 First entity.
   * @param e2 Second entity.
   */
  void ResolveCollision(engine::ecs::Registry& registry,
                        engine::ecs::EntityId e1, engine::ecs::EntityId e2);
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_COLLISION_SYSTEM_H_
