#include "game_logic/systems/ai_system.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/zipper.h"
#include "game_logic/components/ai_component.h"
#include "game_logic/components/player_component.h"

namespace game_logic::systems {

void AISystem::Update(engine::ecs::Registry& registry,
                      engine::time::TimeDelta dt) {
  auto& ais = registry.GetComponents<components::AIComponent>();
  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto& velocities = registry.GetComponents<engine::ecs::VelocityComponent>();
  auto& players = registry.GetComponents<components::PlayerComponent>();

  auto find_nearest_player =
      [&](const engine::math::Vector2f& source_pos) -> engine::math::Vector2f {
    engine::math::Vector2f target_pos = source_pos;
    float min_dist_sq = std::numeric_limits<float>::max();
    bool found = false;

    for (auto [player_pos_comp, player_comp] :
         engine::ecs::Zipper(positions, players)) {
      float dx = player_pos_comp->position.x - source_pos.x;
      float dy = player_pos_comp->position.y - source_pos.y;
      float dist_sq = dx * dx + dy * dy;

      if (dist_sq < min_dist_sq) {
        min_dist_sq = dist_sq;
        target_pos = player_pos_comp->position;
        found = true;
      }
    }

    if (!found) {
      return {source_pos.x - 1000.0f, source_pos.y};
    }
    return target_pos;
  };

  float dt_seconds = dt.as_seconds();

  for (auto [ai, pos, vel] : engine::ecs::Zipper(ais, positions, velocities)) {
    ai->state_timer += dt_seconds;

    switch (ai->behavior) {
      case components::EnemyBehavior::kStraight: {
        vel->velocity.x = -ai->speed;
        vel->velocity.y = 0.0f;
        break;
      }
      case components::EnemyBehavior::kWavePattern: {
        vel->velocity.x = -ai->speed;

        float w = ai->wave_frequency;
        float a = ai->wave_amplitude;
        float t = ai->state_timer;

        vel->velocity.y = a * w * std::cos(w * t);
        break;
      }
      case components::EnemyBehavior::kChasePlayer: {
        engine::math::Vector2f target = find_nearest_player(pos->position);

        engine::math::Vector2f dir = target;
        dir.x -= pos->position.x;
        dir.y -= pos->position.y;

        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (length > 0.0001f) {
          dir.x /= length;
          dir.y /= length;

          vel->velocity = {dir.x * ai->speed, dir.y * ai->speed};
        }
        break;
      }
      default:
        break;
    }
  }
}

}  // namespace game_logic::systems
