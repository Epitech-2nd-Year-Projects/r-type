#include "game_logic/systems/boundary_system.h"

#include <algorithm>
#include <vector>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/zipper.h"
#include "game_logic/components/player_component.h"
#include "game_logic/constants.h"

namespace game_logic::systems {

BoundarySystem::BoundarySystem(float screen_width, float screen_height)
    : screen_width_(screen_width), screen_height_(screen_height) {}

void BoundarySystem::Update(engine::ecs::Registry &registry,
                            engine::time::TimeDelta) {
  auto &positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto &velocities = registry.GetComponents<engine::ecs::VelocityComponent>();
  auto &players =
      registry.GetComponents<game_logic::components::PlayerComponent>();

  for (auto [pos, vel, player] :
       engine::ecs::Zipper(positions, velocities, players)) {
    auto &p = pos->position;
    p.x = std::max(0.0f, std::min(p.x, screen_width_));
    p.y = std::max(0.0f, std::min(p.y, screen_height_));
  }

  std::vector<engine::ecs::EntityId> entities_to_kill;
  auto &player_comps =
      registry.GetComponents<game_logic::components::PlayerComponent>();

  for (auto [idx, pos] : engine::ecs::IndexedZipper(positions)) {
    if (!pos.has_value()) continue;

    if (pos.value().position.x < -200.0f) {
      engine::ecs::EntityId entity = registry.EntityFromIndex(idx);

      if (idx < player_comps.size() && player_comps[idx].has_value()) {
        continue;
      }
      entities_to_kill.push_back(entity);
    }
  }

  for (auto entity : entities_to_kill) {
    registry.KillEntity(entity);
  }
}

}  // namespace game_logic::systems
