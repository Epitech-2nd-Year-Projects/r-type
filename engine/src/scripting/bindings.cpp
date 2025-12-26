#include "engine/scripting/bindings.h"

#include <sol/sol.hpp>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/math/rect.h"
#include "engine/math/vector2.h"
#include "engine/render/color.h"
#include "engine/scripting/script_system.h"
#include "engine/util/logging.h"

namespace engine::scripting {

void BindTypes(sol::state& lua) {
  lua.new_usertype<math::Vector2<float>>(
      "Vector2",
      sol::constructors<math::Vector2<float>(),
                        math::Vector2<float>(float, float)>(),
      "x", &math::Vector2<float>::x, "y", &math::Vector2<float>::y);

  lua.new_usertype<math::RectF>(
      "Rect",
      sol::constructors<math::RectF(),
                        math::RectF(float, float, float, float)>(),
      "x",
      sol::property([](math::RectF& r) { return r.top_left_x_; },
                    [](math::RectF& r, float v) { r.top_left_x_ = v; }),
      "y",
      sol::property([](math::RectF& r) { return r.top_left_y_; },
                    [](math::RectF& r, float v) { r.top_left_y_ = v; }),
      "w",
      sol::property([](math::RectF& r) { return r.width_; },
                    [](math::RectF& r, float v) { r.width_ = v; }),
      "h",
      sol::property([](math::RectF& r) { return r.height_; },
                    [](math::RectF& r, float v) { r.height_ = v; }));

  lua.new_usertype<render::Color>(
      "Color",
      sol::factories(
          [](uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
            return render::Color::FromBytes(r, g, b, a);
          },
          [](uint8_t r, uint8_t g, uint8_t b) {
            return render::Color::FromBytes(r, g, b);
          },
          []() { return render::Color(); }),
      "r", &render::Color::r, "g", &render::Color::g, "b", &render::Color::b,
      "a", &render::Color::a);

  lua.new_usertype<ecs::EntityId>("EntityId");

  lua.new_usertype<ecs::PositionComponent>(
      "PositionComponent",
      sol::constructors<ecs::PositionComponent(float, float),
                        ecs::PositionComponent(math::Vector2<float>)>(),
      "position", &ecs::PositionComponent::position);

  lua.new_usertype<ecs::VelocityComponent>(
      "VelocityComponent",
      sol::constructors<ecs::VelocityComponent(float, float),
                        ecs::VelocityComponent(math::Vector2<float>)>(),
      "velocity", &ecs::VelocityComponent::velocity);

  lua.new_enum("SystemType", "Variable", ecs::SystemType::Variable, "Fixed",
               ecs::SystemType::Fixed);

  lua.new_usertype<ecs::Registry>(
      "Registry", sol::no_constructor, "create_entity",
      &ecs::Registry::SpawnEntity, "kill_entity", &ecs::Registry::KillEntity,

      "add_position",
      [](ecs::Registry& r, ecs::EntityId e, float x, float y) {
        return r.EmplaceComponent<ecs::PositionComponent>(e, x, y);
      },
      "add_velocity",
      [](ecs::Registry& r, ecs::EntityId e, float x, float y) {
        return r.EmplaceComponent<ecs::VelocityComponent>(e, x, y);
      },

      "get_position",
      [](ecs::Registry& r,
         ecs::EntityId e) -> std::optional<ecs::PositionComponent> {
        try {
          auto& sparse = r.GetComponents<ecs::PositionComponent>();
          if (e < sparse.size() && sparse[e].has_value()) {
            return sparse[e].value();
          }
        } catch (const std::exception& ex) {
          ENGINE_LOG_ERROR("Lua get_position error: {}", ex.what());
        } catch (...) {
          ENGINE_LOG_ERROR("Lua get_position error: Unknown exception");
        }
        return std::nullopt;
      },
      "get_velocity",
      [](ecs::Registry& r,
         ecs::EntityId e) -> std::optional<ecs::VelocityComponent> {
        try {
          auto& sparse = r.GetComponents<ecs::VelocityComponent>();
          if (e < sparse.size() && sparse[e].has_value()) {
            return sparse[e].value();
          }
        } catch (const std::exception& ex) {
          ENGINE_LOG_ERROR("Lua get_velocity error: {}", ex.what());
        } catch (...) {
          ENGINE_LOG_ERROR("Lua get_velocity error: Unknown exception");
        }
        return std::nullopt;
      },

      "register_system",
      [](ecs::Registry& self, sol::function fn,
         sol::optional<ecs::SystemType> type,
         sol::optional<ecs::SystemPriority> priority) {
        auto sys = std::make_shared<engine::scripting::ScriptSystem>(fn);
        self.AddSystemClass(sys, type.value_or(ecs::SystemType::Variable),
                            priority.value_or(ecs::kDefaultPriority));
      });

  ENGINE_LOG_INFO("Lua bindings types initialized (Math + ECS + Systems)");
}

void BindRegistry(sol::state& lua, engine::ecs::Registry& registry) {
  lua["registry"] = std::ref(registry);
  ENGINE_LOG_INFO("Lua global 'registry' bound");
}

}  // namespace engine::scripting
