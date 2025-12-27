#include "game_logic/prefab_binder.h"

#include <sol/sol.hpp>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/scripting/prefab_factory.h"
#include "game_logic/components.h"
#include "game_logic/game_config.h"

namespace game_logic {

void BindGameComponents(engine::scripting::PrefabFactory& factory) {
  factory.RegisterComponent(
      "Position", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                     const sol::object& value) {
        if (value.is<sol::table>()) {
          sol::table t = value;
          float x = t["x"].get_or(0.0f);
          float y = t["y"].get_or(0.0f);
          r.EmplaceComponent<engine::ecs::PositionComponent>(e, x, y);
        }
      });

  factory.RegisterComponent(
      "Velocity", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                     const sol::object& value) {
        if (value.is<sol::table>()) {
          sol::table t = value;
          float x = t["x"].get_or(0.0f);
          float y = t["y"].get_or(0.0f);
          r.EmplaceComponent<engine::ecs::VelocityComponent>(e, x, y);
        }
      });

  factory.RegisterComponent(
      "BoundingBox", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                        const sol::object& value) {
        if (value.is<sol::table>()) {
          sol::table t = value;
          float width = t["width"].get_or(0.0f);
          float height = t["height"].get_or(0.0f);
          float offset_x = t["offset_x"].get_or(0.0f);
          float offset_y = t["offset_y"].get_or(0.0f);
          r.EmplaceComponent<engine::ecs::BoundingBoxComponent>(
              e, offset_x, offset_y, width, height);
        }
      });

  factory.RegisterComponent("Tag", [](engine::ecs::Registry& r,
                                      engine::ecs::EntityId e,
                                      const sol::object& value) {
    if (value.is<std::string>()) {
      r.EmplaceComponent<engine::ecs::TagComponent>(e, value.as<std::string>());
    }
  });

  factory.RegisterComponent(
      "Health", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                   const sol::object& value) {
        if (value.is<int>()) {
          r.EmplaceComponent<components::HealthComponent>(e, value.as<int>());
        } else if (value.is<sol::table>()) {
          sol::table t = value;
          r.EmplaceComponent<components::HealthComponent>(
              e, t["max_health"].get_or(1));
        }
      });

  factory.RegisterComponent(
      "Damageable", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                       const sol::object& value) {
        r.EmplaceComponent<components::DamageableComponent>(e);
      });

  factory.RegisterComponent("ScoreValue", [](engine::ecs::Registry& r,
                                             engine::ecs::EntityId e,
                                             const sol::object& value) {
    if (value.is<int>()) {
      r.EmplaceComponent<components::ScoreValueComponent>(e, value.as<int>());
    }
  });

  factory.RegisterComponent(
      "Sprite", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                   const sol::object& value) {
        if (value.is<sol::table>()) {
          sol::table t = value;
          components::SpriteComponent sprite;
          sprite.texture_path = t["texture"].get_or(std::string(""));
          float w = t["width"].get_or(32.0f);
          float h = t["height"].get_or(32.0f);
          sprite.source_rect = engine::math::RectF(0.0f, 0.0f, w, h);
          sprite.layer = t["layer"].get_or(0);
          sprite.visible = true;
          r.AddComponent<components::SpriteComponent>(e, std::move(sprite));
        }
      });

  factory.RegisterComponent(
      "AI", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
               const sol::object& value) {
        if (value.is<sol::table>()) {
          sol::table t = value;
          components::AIComponent ai;
          std::string type = t["type"].get_or(std::string("Straight"));
          if (type == "Patrol")
            ai.behavior = components::EnemyBehavior::kPatrol;
          else if (type == "WavePattern")
            ai.behavior = components::EnemyBehavior::kWavePattern;
          else if (type == "ChasePlayer")
            ai.behavior = components::EnemyBehavior::kChasePlayer;
          else
            ai.behavior = components::EnemyBehavior::kStraight;

          ai.speed = t["speed"].get_or(100.0f);
          ai.detection_range = t["detection_range"].get_or(0.0f);
          ai.wave_amplitude = t["wave_amplitude"].get_or(0.0f);
          ai.wave_frequency = t["wave_frequency"].get_or(0.0f);

          if (ai.behavior == components::EnemyBehavior::kPatrol) {
            try {
              auto& w = GameConfig::Get().GetWorld();
              ai.patrol_min =
                  engine::math::Vector2f{w.patrol_min_x, w.patrol_min_y};
              ai.patrol_max =
                  engine::math::Vector2f{w.patrol_max_x, w.patrol_max_y};
            } catch (...) {
            }
          }

          r.AddComponent<components::AIComponent>(e, std::move(ai));
        }
      });

  factory.RegisterComponent(
      "Weapon", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                   const sol::object& value) {
        if (value.is<sol::table>()) {
          sol::table t = value;
          components::WeaponComponent weapon;
          weapon.projectile_data.name =
              t["projectile_name"].get_or(std::string("EnemyMissile"));
          weapon.projectile_data.damage = t["damage"].get_or(10);
          weapon.projectile_data.speed = t["projectile_speed"].get_or(300.0f);
          weapon.projectile_data.fire_rate = t["fire_rate"].get_or(1.0f);

          weapon.set_unlimited_ammo();
          weapon.is_trigger_held = true;
          weapon.faction = entities::ProjectileFaction::kEnemy;

          r.AddComponent<components::WeaponComponent>(e, std::move(weapon));
        }
      });

  factory.RegisterComponent(
      "DropsPowerup", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                         const sol::object& value) {
        if (value.is<bool>() && value.as<bool>()) {
          r.EmplaceComponent<components::DropsPowerupComponent>(e);
        }
      });
}

}  // namespace game_logic
