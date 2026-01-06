#include "ecs/render_debug.h"

#include <vector>

#include "ecs/components.h"
#include "engine/ecs/components/bounding_box_component.h"
#include "engine/ecs/zipper.h"

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
  if (!show_colliders && !show_sprite_bounds && !show_velocity &&
      !show_ai_paths) {
    return;
  }

  if (show_colliders) {
    auto& boxes = registry_.GetComponents<engine::ecs::BoundingBoxComponent>();
    auto& positions = registry_.GetComponents<PositionComponent>();

    for (auto [box, pos] : engine::ecs::Zipper(boxes, positions)) {
      auto rect = engine::math::RectF(pos->position.x + box->bounds.top_left_x_,
                                      pos->position.y + box->bounds.top_left_y_,
                                      box->bounds.width_, box->bounds.height_);

      renderer_.DrawRect(rect, engine::render::Color(1.0f, 0.0f, 0.0f));
    }
  }

  if (show_sprite_bounds) {
    auto& sprites = registry_.GetComponents<SpriteComponent>();
    auto& positions = registry_.GetComponents<PositionComponent>();

    for (auto [sprite, pos] : engine::ecs::Zipper(sprites, positions)) {
      auto rect = engine::math::RectF(pos->position.x, pos->position.y,
                                      sprite->source_rect.width_,
                                      sprite->source_rect.height_);
      renderer_.DrawRect(rect, engine::render::Color(0.0f, 0.0f, 1.0f));
    }
  }

  if (show_velocity) {
    auto& velocities = registry_.GetComponents<VelocityComponent>();
    auto& positions = registry_.GetComponents<PositionComponent>();

    for (auto [vel, pos] : engine::ecs::Zipper(velocities, positions)) {
      if (vel->velocity.LengthSquared() > 0.1f) {
        engine::math::Vector2f end = pos->position + (vel->velocity * 0.5f);
        renderer_.DrawLine(pos->position, end, 1.0f,
                           engine::render::Color(1.0f, 1.0f, 0.0f));
      }
    }
  }
}

void RenderDebug::DrawPolyline(
    const std::vector<engine::math::Vector2f>& points,
    const engine::render::Color& color) {
  if (points.size() < 2) {
    return;
  }
  for (size_t i = 0; i < points.size() - 1; ++i) {
    renderer_.DrawLine(points[i], points[i + 1], 1.0f, color);
  }
}

void RenderDebug::DrawAnimatedPolyline(
    const std::vector<engine::math::Vector2f>& points,
    const engine::render::Color& color, float total_time_seconds) {
  if (points.size() < 2) {
    return;
  }

  const float dash_length = 10.0f;
  const float gap_length = 5.0f;
  const float speed = 50.0f;
  const float period = dash_length + gap_length;

  float offset = std::fmod(total_time_seconds * speed, period);

  float current_distance = -offset;

  for (size_t i = 0; i < points.size() - 1; ++i) {
    engine::math::Vector2f start = points[i];
    engine::math::Vector2f end = points[i + 1];
    engine::math::Vector2f direction = end - start;
    float segment_length = direction.Length();

    if (segment_length < 0.001f) continue;

    direction = direction / segment_length;

    float distance_covered = 0.0f;
    while (distance_covered < segment_length) {
      float remaining_in_segment = segment_length - distance_covered;

      float cycle_pos = std::fmod(current_distance, period);

      if (cycle_pos < 0) cycle_pos += period;

      if (cycle_pos < dash_length) {
        float dist_to_gap = dash_length - cycle_pos;
        float draw_len = std::min(remaining_in_segment, dist_to_gap);

        engine::math::Vector2f p1 = start + direction * distance_covered;
        engine::math::Vector2f p2 = p1 + direction * draw_len;

        renderer_.DrawLine(p1, p2, 1.0f, color);

        distance_covered += draw_len;
        current_distance += draw_len;
      } else {
        float dist_to_dash = period - cycle_pos;
        float skip_len = std::min(remaining_in_segment, dist_to_dash);

        distance_covered += skip_len;
        current_distance += skip_len;
      }
    }
  }
}
}  // namespace client::ecs
