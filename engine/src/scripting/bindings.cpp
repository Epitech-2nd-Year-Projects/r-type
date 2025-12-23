#include "engine/scripting/bindings.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/registry.h"
#include "engine/math/rect.h"
#include "engine/math/vector2.h"
#include "engine/render/color.h"
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
      "x", &math::RectF::top_left_x_, "y", &math::RectF::top_left_y_, "w",
      &math::RectF::width_, "h", &math::RectF::height_);

  lua.new_usertype<render::Color>(
      "Color",
      sol::constructors<render::Color(),
                        render::Color(uint8_t, uint8_t, uint8_t, uint8_t)>(),
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
      [](ecs::Registry& r, ecs::EntityId e) -> ecs::PositionComponent* {
        try {
          auto& sparse = r.GetComponents<ecs::PositionComponent>();
          if (e < sparse.size() && sparse[e].has_value()) {
            return &sparse[e].value();
          }
        } catch (...) {
        }
        return nullptr;
      },
      "get_velocity",
      [](ecs::Registry& r, ecs::EntityId e) -> ecs::VelocityComponent* {
        try {
          auto& sparse = r.GetComponents<ecs::VelocityComponent>();
          if (e < sparse.size() && sparse[e].has_value()) {
            return &sparse[e].value();
          }
        } catch (...) {
        }
        return nullptr;
      });

  ENGINE_LOG_INFO("Lua bindings types initialized (Math + ECS)");
}

void BindRegistry(sol::state& lua, engine::ecs::Registry& registry) {
  lua["registry"] = std::ref(registry);
  ENGINE_LOG_INFO("Lua global 'registry' bound");
}

}  // namespace engine::scripting
