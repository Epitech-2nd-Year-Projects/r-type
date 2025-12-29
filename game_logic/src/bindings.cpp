#include "game_logic/bindings.h"

#include <limits>
#include <tuple>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/ecs/zipper.h"
#include "game_logic/components/ai_component.h"
#include "engine/ecs/components/tag_component.h"
#include "game_logic/components/damageable_component.h"
#include "game_logic/components/powerup_drop_component.h"
#include "game_logic/components/health_component.h"
#include "game_logic/components/player_component.h"
#include "game_logic/components/score_value_component.h"
#include "game_logic/components/sprite_component.h"
#include "game_logic/components/weapon_component.h"
#include "engine/ecs/components/position_component.h"
#include "game_logic/game_instance.h"

namespace game_logic {

void BindRuntimeTypes(sol::state& lua,
                      engine::scripting::PrefabFactory& factory) {
  lua.new_usertype<components::AIComponent>(
      "AIComponent", "behavior_name", &components::AIComponent::behavior_name,
      "speed", &components::AIComponent::speed, "patrol_min",
      &components::AIComponent::patrol_min, "patrol_max",
      &components::AIComponent::patrol_max, "detection_range",
      &components::AIComponent::detection_range, "state_timer",
      &components::AIComponent::state_timer, "wave_amplitude",
      &components::AIComponent::wave_amplitude, "wave_frequency",
      &components::AIComponent::wave_frequency);


  // HealthComponent
  lua.new_usertype<components::HealthComponent>(
      "HealthComponent", "current_health",
      &components::HealthComponent::current_health, "max_health",
      &components::HealthComponent::max_health, "invulnerable",
      &components::HealthComponent::invulnerable, "last_attacker_id",
      &components::HealthComponent::last_attacker_id, "take_damage",
      &components::HealthComponent::take_damage, "is_alive",
      &components::HealthComponent::is_alive);

  // DamageableComponent
  lua.new_usertype<components::DamageableComponent>(
      "DamageableComponent", "damage", &components::DamageableComponent::damage,
      "faction", &components::DamageableComponent::faction, "owner_id",
      &components::DamageableComponent::owner_id);

  // PlayerComponent
  lua.new_usertype<components::PlayerComponent>(
      "PlayerComponent", "player_id", &components::PlayerComponent::player_id,
      "score", &components::PlayerComponent::score, "lives",
      &components::PlayerComponent::lives, "player_slot",
      &components::PlayerComponent::player_slot);

  // ScoreValueComponent
  lua.new_usertype<components::ScoreValueComponent>(
      "ScoreValueComponent", "points",
      &components::ScoreValueComponent::points, "claimed",
      &components::ScoreValueComponent::claimed);

  // DropsPowerupComponent
  lua.new_usertype<components::DropsPowerupComponent>("DropsPowerupComponent");

  // TagComponent (Engine)
  lua.new_usertype<engine::ecs::TagComponent>(
      "TagComponent", "tag", &engine::ecs::TagComponent::tag);

  // WeaponComponent
  lua.new_usertype<components::WeaponComponent>(
      "WeaponComponent", "weapon_script",
      &components::WeaponComponent::weapon_script, "is_trigger_held",
      &components::WeaponComponent::is_trigger_held, "fire_rate",
      &components::WeaponComponent::fire_rate, "cooldown_remaining",
      sol::property(
          [](components::WeaponComponent& w) {
            return w.cooldown_remaining.as_seconds();
          },
          [](components::WeaponComponent& w, float v) {
            w.cooldown_remaining = engine::time::TimeDelta::from_seconds(v);
          }),
      "is_big_trigger_held", &components::WeaponComponent::is_big_trigger_held,
      "big_shot_cooldown_remaining",
      sol::property(
          [](components::WeaponComponent& w) {
            return w.big_shot_cooldown_remaining.as_seconds();
          },
          [](components::WeaponComponent& w, float v) {
            w.big_shot_cooldown_remaining =
                engine::time::TimeDelta::from_seconds(v);
          }),
      "projectile_prefab", &components::WeaponComponent::projectile_prefab,
      "projectile_speed", &components::WeaponComponent::projectile_speed,
      "big_projectile_prefab",
      &components::WeaponComponent::big_projectile_prefab,
      "big_projectile_speed",
      &components::WeaponComponent::big_projectile_speed, "faction",
      &components::WeaponComponent::faction, "can_fire",
      &components::WeaponComponent::can_fire, "fire",
      &components::WeaponComponent::fire, "can_fire_big",
      &components::WeaponComponent::can_fire_big, "fire_big",
      &components::WeaponComponent::fire_big);

  // --- Helper Getters ---

  lua.set_function("GetHealth",
                   [](engine::ecs::Registry& r, engine::ecs::EntityId e)
                       -> components::HealthComponent* {
                     auto& pool =
                         r.GetComponents<components::HealthComponent>();
                     if (e < pool.size() && pool[e]) return &pool[e].value();
                     return nullptr;
                   });

  lua.set_function("GetDamageable",
                   [](engine::ecs::Registry& r, engine::ecs::EntityId e)
                       -> components::DamageableComponent* {
                     auto& pool =
                         r.GetComponents<components::DamageableComponent>();
                     if (e < pool.size() && pool[e]) return &pool[e].value();
                     return nullptr;
                   });

  lua.set_function("GetPlayer",
                   [](engine::ecs::Registry& r, engine::ecs::EntityId e)
                       -> components::PlayerComponent* {
                     auto& pool =
                         r.GetComponents<components::PlayerComponent>();
                     if (e < pool.size() && pool[e]) return &pool[e].value();
                     return nullptr;
                   });

  lua.set_function("GetTag",
                   [](engine::ecs::Registry& r, engine::ecs::EntityId e)
                       -> engine::ecs::TagComponent* {
                     auto& pool = r.GetComponents<engine::ecs::TagComponent>();
                     if (e < pool.size() && pool[e]) return &pool[e].value();
                     return nullptr;
                   });

  lua.set_function("GetScoreValue",
                   [](engine::ecs::Registry& r, engine::ecs::EntityId e)
                       -> components::ScoreValueComponent* {
                     auto& pool =
                         r.GetComponents<components::ScoreValueComponent>();
                     if (e < pool.size() && pool[e]) return &pool[e].value();
                     return nullptr;
                   });

  lua.set_function("GetDropsPowerup",
                   [](engine::ecs::Registry& r, engine::ecs::EntityId e)
                       -> components::DropsPowerupComponent* {
                     auto& pool =
                         r.GetComponents<components::DropsPowerupComponent>();
                     if (e < pool.size() && pool[e]) return &pool[e].value();
                     return nullptr;
                   });

  lua.set_function("GetPosition",
                   [](engine::ecs::Registry& r, engine::ecs::EntityId e)
                       -> engine::ecs::PositionComponent* {
                     auto& pool =
                         r.GetComponents<engine::ecs::PositionComponent>();
                     if (e < pool.size() && pool[e]) return &pool[e].value();
                     return nullptr;
                   });

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

  lua.set_function(
      "Spawn",
      [&factory](engine::ecs::Registry& registry,
                 const std::string& prefab_name, float x,
                 float y) -> std::optional<engine::ecs::EntityId> {
        auto opt_ent = factory.Spawn(registry, prefab_name);
        if (opt_ent) {
          registry.EmplaceComponent<engine::ecs::PositionComponent>(*opt_ent, x,
                                                                    y);
          return *opt_ent;
        }
        return std::nullopt;
      });
}

}  // namespace game_logic
