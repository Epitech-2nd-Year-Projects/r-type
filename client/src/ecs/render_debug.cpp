#include "ecs/render_debug.h"

namespace client::ecs {

RenderDebug::RenderDebug(engine::ecs::Registry& registry,
                         engine::render::Renderer2D& renderer)
    : registry_(registry), renderer_(renderer) {
  RegisterComponents();
}

void RenderDebug::RegisterComponents() {
  registry_.RegisterComponent<ecs::PositionComponent>();
  registry_.RegisterComponent<ecs::NetworkedEntityComponent>();
  registry_.RegisterComponent<engine::ecs::BoundingBoxComponent>();
  registry_.RegisterComponent<ecs::SpriteComponent>();
  registry_.RegisterComponent<ecs::VelocityComponent>();
}

void RenderDebug::Draw() {
  if (!show_colliders && !show_sprite_bounds && !show_velocity) {
    return;
  }

  const auto& positions = registry_.GetComponents<ecs::PositionComponent>();
  const auto& nets = registry_.GetComponents<ecs::NetworkedEntityComponent>();
  const auto& hitboxes =
      registry_.GetComponents<engine::ecs::BoundingBoxComponent>();
  const auto& sprites = registry_.GetComponents<ecs::SpriteComponent>();
  const auto& vels = registry_.GetComponents<ecs::VelocityComponent>();

  const engine::render::Color box_color =
      engine::render::Color::FromBytes(255, 60, 60, 180);
  const engine::render::Color sprite_color =
      engine::render::Color::FromBytes(60, 60, 255, 180);
  const engine::render::Color vel_color =
      engine::render::Color::FromBytes(255, 255, 0, 200);

  const std::size_t count = positions.size();

  for (std::size_t i = 0; i < count; ++i) {
    if (!positions[i].has_value()) {
      continue;
    }
    if (i >= nets.size() || !nets[i].has_value()) {
      continue;
    }

    const auto& pos = positions[i]->render_position;

    if (show_colliders && i < hitboxes.size() && hitboxes[i].has_value()) {
      const auto& bounds = hitboxes[i]->bounds;
      const engine::math::RectF rect{pos.x + bounds.top_left_x_,
                                     pos.y + bounds.top_left_y_, bounds.width_,
                                     bounds.height_};
      renderer_.DrawRect(rect, box_color);
    }

    if (show_sprite_bounds && i < sprites.size() && sprites[i].has_value()) {
      const auto& sprite = sprites[i].value();
      auto rect = sprite.source_rect;
      const engine::math::Vector2f tl{pos.x, pos.y};
      const engine::math::Vector2f tr{pos.x + rect.width_, pos.y};
      const engine::math::Vector2f bl{pos.x, pos.y + rect.height_};
      const engine::math::Vector2f br{pos.x + rect.width_,
                                      pos.y + rect.height_};

      renderer_.DrawLine(tl, tr, 1.0f, sprite_color);
      renderer_.DrawLine(tr, br, 1.0f, sprite_color);
      renderer_.DrawLine(br, bl, 1.0f, sprite_color);
      renderer_.DrawLine(bl, tl, 1.0f, sprite_color);
    }

    if (show_velocity && i < vels.size() && vels[i].has_value()) {
      const auto& vel = vels[i]->velocity;
      if (vel.Length() > 0.1f) {
        engine::math::Vector2f end = pos + vel * 0.5f;
        renderer_.DrawLine(pos, end, 2.0f, vel_color);
      }
    }
  }
}

}  // namespace client::ecs
