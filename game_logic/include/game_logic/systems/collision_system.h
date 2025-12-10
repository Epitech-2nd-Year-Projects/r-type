#ifndef GAME_LOGIC_SYSTEMS_COLLISION_SYSTEM_H_
#define GAME_LOGIC_SYSTEMS_COLLISION_SYSTEM_H_

#include <functional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/ecs/entity_id.h"
#include "engine/ecs/system.h"
#include "engine/math/rect.h"
#include "engine/time/time_delta.h"

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
class CollisionSystem : public engine::ecs::ISystem {
 public:
  static constexpr std::string_view kPlayerTag = "Player";
  static constexpr std::string_view kEnemyTag = "Enemy";
  static constexpr std::uint32_t kCrashDamage = 100;

  CollisionSystem(float cell_size = 100.0f);
  ~CollisionSystem() override = default;

  /**
   * @brief Update collisions. Rebuilds grid and checks collisions.
   */
  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;

 private:
  struct GridKey {
    int x;
    int y;

    bool operator==(const GridKey& other) const {
      return x == other.x && y == other.y;
    }
  };

  struct GridKeyHash {
    std::size_t operator()(const GridKey& k) const {
      return std::hash<int>()(k.x) ^
             (std::hash<int>()(k.y) + 0x9e3779b9 +
              (std::hash<int>()(k.x) << 6) + (std::hash<int>()(k.x) >> 2));
    }
  };

  using EntityList = std::vector<engine::ecs::EntityId>;
  using SpatialGrid = std::unordered_map<GridKey, EntityList, GridKeyHash>;

  float cell_size_;
  SpatialGrid grid_;

  /**
   * @brief Helper to resolve simple AABB interaction
   */
  bool CheckOneWayCollision(const engine::math::RectF& a,
                            const engine::math::RectF& b);

  /**
   * @brief Insert entity into all relevant grid cells
   */
  void InsertIntoGrid(engine::ecs::EntityId entity,
                      const engine::math::RectF& bounds);
};

}  // namespace game_logic::systems

#endif  // GAME_LOGIC_SYSTEMS_COLLISION_SYSTEM_H_
