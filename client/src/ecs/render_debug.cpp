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
}

void RenderDebug::Draw() {
  if (!enabled_) {
    return;
  }

  const auto& positions = registry_.GetComponents<ecs::PositionComponent>();
  const auto& nets = registry_.GetComponents<ecs::NetworkedEntityComponent>();
  const auto& hitboxes =
      registry_.GetComponents<engine::ecs::BoundingBoxComponent>();

  const engine::render::Color box_color =
      engine::render::Color::FromBytes(255, 60, 60, 180);
  const std::size_t hitbox_count = hitboxes.size();

  for (std::size_t i = 0; i < hitbox_count; ++i) {
    if (i >= positions.size() || !positions[i].has_value()) {
      continue;
    }
    if (i >= nets.size() || !nets[i].has_value()) {
      continue;
    }
    if (!hitboxes[i].has_value()) {
      continue;
    }

    const auto& pos = positions[i]->render_position;
    const auto& bounds = hitboxes[i]->bounds;
    const engine::math::RectF rect{pos.x + bounds.top_left_x_,
                                   pos.y + bounds.top_left_y_, bounds.width_,
                                   bounds.height_};
    renderer_.DrawRect(rect, box_color);
  }
}

}  // namespace client::ecs
