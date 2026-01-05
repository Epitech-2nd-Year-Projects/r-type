#include "systems/debug_path_system.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "ecs/archetype_registry.h"
#include "ecs/components.h"
#include "engine/ecs/indexed_zipper.h"
#include "engine/math/vector2.h"

namespace client::systems {

DebugPathSystem::DebugPathSystem(engine::ecs::Registry& registry,
                                 client::ecs::RenderDebug& render_debug)
    : registry_(registry), render_debug_(render_debug) {}

void DebugPathSystem::Update(engine::time::TimeDelta dt) {
  if (!render_debug_.show_ai_paths) {
    return;
  }

  total_time_ += dt.as_seconds();

  auto& networked =
      registry_.GetComponents<client::ecs::NetworkedEntityComponent>();
  auto& positions = registry_.GetComponents<client::ecs::PositionComponent>();
  auto& velocities = registry_.GetComponents<client::ecs::VelocityComponent>();
  auto& healths = registry_.GetComponents<client::ecs::HealthComponent>();
  auto& sprites = registry_.GetComponents<client::ecs::SpriteComponent>();
  auto& players = registry_.GetComponents<client::ecs::LocalPlayerTag>();

  engine::math::Vector2f player_pos;
  bool player_found = false;
  for (auto [idx, pos, tag] : engine::ecs::IndexedZipper(positions, players)) {
    player_pos = pos->position;
    player_found = true;
    break;
  }

  for (auto [entity_idx, net, pos, vel] :
       engine::ecs::IndexedZipper(networked, positions, velocities)) {
    const auto& archetype_registry = client::ecs::ArchetypeRegistry::Get();
    auto start_pos = pos->position;
    auto velocity = vel->velocity;
    std::vector<engine::math::Vector2f> points;

    if (archetype_registry.IsEnemy(net->type_code)) {
      if (entity_idx < sprites.size() && sprites[entity_idx].has_value()) {
        const auto& sprite = sprites[entity_idx].value();
        start_pos.x += sprite.source_rect.width_ / 2.0f;
        start_pos.y += sprite.source_rect.height_ / 2.0f;
      }

      bool is_tank = false;
      bool is_bomber = false;

      if (entity_idx < healths.size() && healths[entity_idx].has_value()) {
        if (healths[entity_idx]->max >= 150)
          is_tank = true;
        else if (healths[entity_idx]->max >= 20)
          is_bomber = true;
      }

      bool is_interceptor = velocity.Length() > 170.0f;

      if (is_tank) {
        points.push_back({start_pos.x, 100.0f});
        points.push_back({start_pos.x, 500.0f});
        render_debug_.DrawAnimatedPolyline(
            points, engine::render::Color(1.0f, 0.5f, 0.0f), total_time_);
      } else if (is_bomber) {
        points.push_back(start_pos);
        float amplitude = 50.0f;
        float frequency = 2.0f;
        float speed = 100.0f;
        float vy = velocity.y;
        auto& state = entity_states_[entity_idx];
        float phase = 0.0f;
        float ratio = 0.0f;
        if (std::abs(amplitude) > 0.001f) {
          ratio = std::clamp(vy / amplitude, -1.0f, 1.0f);
        }
        float asin_phase = std::asin(ratio);
        float candidate1 = asin_phase;
        float candidate2 = 3.14159265f - asin_phase;
        const float TWO_PI = 6.2831853f;

        if (!state.initialized) {
          float accel = vy - state.last_vy;
          if (accel < 0) {
            phase = candidate2;
          } else {
            phase = candidate1;
          }
          state.initialized = true;
        } else {
          float predicted_phase = state.phase + frequency * dt.as_seconds();
          auto circular_dist = [&](float a, float b) {
            float d = std::fmod(std::abs(a - b), TWO_PI);
            return d > 3.14159f ? TWO_PI - d : d;
          };
          if (circular_dist(candidate1, predicted_phase) <
              circular_dist(candidate2, predicted_phase)) {
            phase = candidate1;
          } else {
            phase = candidate2;
          }
          float diff = phase - predicted_phase;
          diff = std::fmod(diff + 3.14159f, TWO_PI);
          if (diff < 0) diff += TWO_PI;
          diff -= 3.14159f;

          phase = predicted_phase + diff;
        }

        state.phase = phase;
        state.last_vy = vy;

        engine::math::Vector2f cursor = start_pos;
        float dt_step = 0.1f;

        for (int i = 1; i <= 30; ++i) {
          float sim_t = phase + i * dt_step * frequency;
          float vy_pred = std::sin(sim_t) * amplitude;
          float vx_pred = -speed;

          cursor.x += vx_pred * dt_step;
          cursor.y += vy_pred * dt_step;

          points.push_back(cursor);
        }
        render_debug_.DrawAnimatedPolyline(
            points, engine::render::Color(1.0f, 0.0f, 1.0f), total_time_);
      } else if (is_interceptor) {
        if (player_found) {
          points.push_back(start_pos);
          points.push_back(player_pos);
          render_debug_.DrawAnimatedPolyline(
              points, engine::render::Color(1.0f, 0.0f, 0.0f), total_time_);
        } else {
          points.push_back(start_pos);
          points.push_back(start_pos + (velocity * 3.0f));
          render_debug_.DrawAnimatedPolyline(
              points, engine::render::Color(0.5f, 0.0f, 0.0f), total_time_);
        }
      } else {
        points.push_back(start_pos);
        points.push_back(start_pos + (velocity * 3.0f));
        render_debug_.DrawAnimatedPolyline(
            points, engine::render::Color(0.0f, 1.0f, 1.0f), total_time_);
      }
    }
  }
}

}  // namespace client::systems
