#include "game_logic/prefab_binder.h"

#include <sol/sol.hpp>

#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/components/lifetime_component.h"
#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/scripting/prefab_factory.h"
#include "game_logic/components.h"
#include "game_logic/components/powerup_component.h"
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
      "Lifetime", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                     const sol::object& value) {
        if (value.is<double>()) {
          r.EmplaceComponent<engine::ecs::LifetimeComponent>(
              e, engine::time::TimeDelta::from_seconds(value.as<double>()));
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
        if (value.is<sol::table>()) {
          sol::table t = value;
          std::uint32_t damage = t["damage"].get_or(10);
          std::uint8_t faction = t["faction"].get_or(0);
          bool friendly_fire = t["friendly_fire"].get_or(false);
          std::uint32_t owner_id = t["owner_id"].get_or(0);
          r.EmplaceComponent<components::DamageableComponent>(e, owner_id,
                                                              damage, faction);
          r.GetComponents<components::DamageableComponent>()[e]->friendly_fire =
              friendly_fire;
        } else {
          r.EmplaceComponent<components::DamageableComponent>(e);
        }
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

          if (t["tint"].valid()) {
            sol::table tint = t["tint"];
            sprite.tint.r = tint["r"].get_or(255);
            sprite.tint.g = tint["g"].get_or(255);
            sprite.tint.b = tint["b"].get_or(255);
            sprite.tint.a = tint["a"].get_or(255);
          }

          r.AddComponent<components::SpriteComponent>(e, std::move(sprite));
        }
      });

  factory.RegisterComponent(
      "AI", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
               const sol::object& value) {
        if (value.is<sol::table>()) {
          sol::table t = value;
          std::string behavior = t["behavior"].get_or(std::string("Straight"));
          float speed = t["speed"].get_or(100.0f);

          components::AIComponent ai(std::move(behavior), speed);

          if (t["patrol_min_x"].valid()) {
            ai.patrol_min = engine::math::Vector2f{
                t["patrol_min_x"], t["patrol_min_y"].get_or(0.0f)};
          }

          if (t["patrol_max_x"].valid()) {
            ai.patrol_max = engine::math::Vector2f{
                t["patrol_max_x"], t["patrol_max_y"].get_or(0.0f)};
          }

          ai.detection_range = t["detection_range"].get_or(0.0f);
          ai.wave_amplitude = t["wave_amplitude"].get_or(0.0f);
          ai.wave_frequency = t["wave_frequency"].get_or(0.0f);

          r.AddComponent<components::AIComponent>(e, std::move(ai));
        }
      });

  factory.RegisterComponent(
      "Weapon", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                   const sol::object& value) {
        if (value.is<sol::table>()) {
          sol::table t = value;
          components::WeaponComponent weapon;
          weapon.projectile_prefab =
              t["projectile_name"].get_or(std::string("PlayerMissile"));
          weapon.projectile_speed = t["projectile_speed"].get_or(400.0f);
          weapon.fire_rate = t["fire_rate"].get_or(2.0f);
          weapon.big_projectile_prefab =
              t["big_projectile_name"].get_or(std::string(""));
          weapon.big_projectile_speed = t["big_projectile_speed"].get_or(0.0f);

          weapon.set_unlimited_ammo();
          weapon.is_trigger_held = t["trigger_held"].get_or(false);
          weapon.weapon_script = t["weapon_script"].get_or(std::string(""));
          weapon.faction =
              static_cast<entities::ProjectileFaction>(t["faction"].get_or(0));

          r.AddComponent<components::WeaponComponent>(e, std::move(weapon));
        }
      });

  factory.RegisterComponent(
      "PlayerValue", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                        const sol::object& value) {
        if (value.is<sol::table>()) {
          sol::table t = value;
          components::PlayerComponent player;
          player.lives = t["lives"].get_or(3);
          player.score = t["score"].get_or(0);
          r.AddComponent<components::PlayerComponent>(e, std::move(player));
        }
      });

  factory.RegisterComponent(
      "DropsPowerup", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                         const sol::object& value) {
        if (value.is<bool>() && value.as<bool>()) {
          r.EmplaceComponent<components::DropsPowerupComponent>(e);
        }
      });

  factory.RegisterComponent(
      "Powerup", [](engine::ecs::Registry& r, engine::ecs::EntityId e,
                    const sol::object& value) {
        if (value.is<sol::table>()) {
          sol::table t = value;
          components::PowerupComponent powerup;
          std::string type = t["type"].get_or(std::string("Health"));

          if (type == "Health")
            powerup.type = components::PowerupType::kHealth;
          else if (type == "WeaponUpgrade")
            powerup.type = components::PowerupType::kWeaponUpgrade;
          else if (type == "SpeedBoost")
            powerup.type = components::PowerupType::kSpeedBoost;
          else if (type == "Shield")
            powerup.type = components::PowerupType::kShield;
          else if (type == "ExtraLife")
            powerup.type = components::PowerupType::kExtraLife;
          else if (type == "Score")
            powerup.type = components::PowerupType::kScore;

          powerup.value = t["value"].get_or(0);
          r.AddComponent<components::PowerupComponent>(e, std::move(powerup));
        }
      });
}

}  // namespace game_logic
