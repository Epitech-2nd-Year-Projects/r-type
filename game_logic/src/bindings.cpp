#include "game_logic/bindings.h"

#include <limits>
#include <tuple>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/zipper.h"
#include "game_logic/components/ai_component.h"
#include "game_logic/components/health_component.h"
#include "game_logic/components/player_component.h"
#include "game_logic/components/score_value_component.h"
#include "game_logic/components/sprite_component.h"
#include "game_logic/components/weapon_component.h"
#include "game_logic/game_instance.h"

namespace game_logic {

void BindRuntimeTypes(sol::state& lua) {
  lua.new_usertype<components::AIComponent>(
      "AIComponent", "behavior_name", &components::AIComponent::behavior_name,
      "speed", &components::AIComponent::speed, "patrol_min",
      &components::AIComponent::patrol_min, "patrol_max",
      &components::AIComponent::patrol_max, "detection_range",
      &components::AIComponent::detection_range, "state_timer",
      &components::AIComponent::state_timer, "wave_amplitude",
      &components::AIComponent::wave_amplitude, "wave_frequency",
      &components::AIComponent::wave_frequency);

  lua.new_usertype<components::HealthComponent>(
      "HealthComponent", "health", &components::HealthComponent::current_health,
      "max_health", &components::HealthComponent::max_health);

  lua.set_function(
      "GetNearestPlayerPosition",
      [](engine::ecs::Registry& registry,
         const engine::math::Vector2f& pos) -> std::tuple<bool, float, float> {
        float min_dist_sq = std::numeric_limits<float>::max();
        bool found = false;
        float target_x = 0.0f;
        float target_y = 0.0f;

        auto& players = registry.GetComponents<components::PlayerComponent>();
        auto& positions =
            registry.GetComponents<engine::ecs::PositionComponent>();

        for (auto [player, p] : engine::ecs::Zipper(players, positions)) {
          float dx = p->position.x - pos.x;
          float dy = p->position.y - pos.y;
          float dist_sq = dx * dx + dy * dy;
          if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            found = true;
            target_x = p->position.x;
            target_y = p->position.y;
          }
        }
        return std::make_tuple(found, target_x, target_y);
      });
}

}  // namespace game_logic
