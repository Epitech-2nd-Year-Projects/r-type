#ifndef RIFT_CLIENT_ECS_COMPONENTS_H_
#define RIFT_CLIENT_ECS_COMPONENTS_H_

#include <cstdint>

#include "engine/math/vector2.h"
#include "engine/render/color.h"
#include "rift/components/fighter_component.h"

namespace rift::client::ecs {

struct NetworkedEntityComponent {
  std::uint32_t network_id{0};
  std::uint16_t type_code{0};
  std::uint32_t last_snapshot{0};
  std::uint8_t flags{0};

  NetworkedEntityComponent() = default;
  NetworkedEntityComponent(std::uint32_t id, std::uint16_t type,
                           std::uint32_t snapshot, std::uint8_t f = 0)
      : network_id(id), type_code(type), last_snapshot(snapshot), flags(f) {}
};

struct PositionComponent {
  engine::math::Vector2f position{0.0f, 0.0f};
  engine::math::Vector2f previous_position{0.0f, 0.0f};
  engine::math::Vector2f render_position{0.0f, 0.0f};

  PositionComponent() = default;
  PositionComponent(float x, float y)
      : position(x, y), previous_position(x, y), render_position(x, y) {}
};

struct VelocityComponent {
  engine::math::Vector2f velocity{0.0f, 0.0f};

  VelocityComponent() = default;
  VelocityComponent(float vx, float vy) : velocity(vx, vy) {}
};

struct FighterTag {};

struct LocalFighterTag {};

struct FighterStateComponent {
  std::uint32_t player_id{0};
  std::uint8_t slot{0};
  std::uint8_t rounds_won{0};
  bool facing_right{true};

  FighterStateComponent() = default;
  FighterStateComponent(std::uint32_t id, std::uint8_t s)
      : player_id(id), slot(s), facing_right(s == 0) {}
};

struct CombatDisplayComponent {
  components::CombatState state{components::CombatState::kIdle};
  std::uint32_t state_timer_ms{0};
  components::AttackType attack_type{components::AttackType::kNone};

  CombatDisplayComponent() = default;
};

struct HealthBarComponent {
  std::uint32_t current{100};
  std::uint32_t max{100};

  HealthBarComponent() = default;
  HealthBarComponent(std::uint32_t curr, std::uint32_t m)
      : current(curr), max(m) {}

  float Percent() const {
    return max > 0 ? static_cast<float>(current) / static_cast<float>(max)
                   : 0.0f;
  }
};

struct StaminaBarComponent {
  float current{100.0f};
  float max{100.0f};

  StaminaBarComponent() = default;
  StaminaBarComponent(float curr, float m) : current(curr), max(m) {}

  float Percent() const {
    return max > 0.0f ? current / max : 0.0f;
  }
};

struct FighterRenderComponent {
  float width{60.0f};
  float height{120.0f};
  engine::render::Color color{engine::render::Color::FromBytes(50, 100, 255)};

  FighterRenderComponent() = default;
  FighterRenderComponent(float w, float h, engine::render::Color c)
      : width(w), height(h), color(c) {}
};

struct SnapshotInterpolationComponent {
  std::uint64_t previous_snapshot_ms{0};
  std::uint64_t last_snapshot_ms{0};

  SnapshotInterpolationComponent() = default;
  SnapshotInterpolationComponent(std::uint64_t last, std::uint64_t prev)
      : previous_snapshot_ms(prev), last_snapshot_ms(last) {}
};

}  // namespace rift::client::ecs

#endif  // RIFT_CLIENT_ECS_COMPONENTS_H_
