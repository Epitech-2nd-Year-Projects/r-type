#include "ecs/animation_factory.h"

#include <vector>

#include "ecs/components.h"
#include "engine/math/rect.h"

namespace client::ecs {

namespace {

constexpr std::size_t kBossColumns = 4u;
constexpr std::size_t kBossRows = 9u;
constexpr std::uint32_t kBossHealthThreshold = 500u;

std::vector<engine::math::RectF> BuildVerticalFrames(float width, float height,
                                                     std::size_t count) {
  std::vector<engine::math::RectF> frames;
  frames.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    frames.emplace_back(0.0f, static_cast<float>(i) * height, width, height);
  }
  return frames;
}

std::vector<engine::math::RectF> PlayerFrames() {
  return BuildVerticalFrames(26.0f, 21.0f, 3u);
}

std::vector<engine::math::RectF> EnemyFrames() {
  return BuildVerticalFrames(29.0f, 29.0f, 5u);
}

std::vector<engine::math::RectF> BossFrames() {
  constexpr float kStrideX = 162.0f;
  constexpr float kStrideY = 213.2f;
  constexpr float kOffsetX = 19.0f;
  constexpr float kFrameWidth = 162.0f;
  constexpr float kFrameHeight = 212.0f;
  std::vector<engine::math::RectF> frames;

  frames.reserve(kBossColumns * kBossRows);
  for (std::size_t row = 0; row < kBossRows; ++row) {
    for (std::size_t col = 0; col < kBossColumns; ++col) {
      frames.emplace_back(static_cast<float>(col) * kStrideX + kOffsetX,
                          static_cast<float>(row) * kStrideY, kFrameWidth,
                          kFrameHeight);
    }
  }
  return frames;
}

std::vector<engine::math::RectF> MissileFrames() {
  return BuildVerticalFrames(19.0f, 6.0f, 2u);
}

bool IsBoss(engine::ecs::Registry &registry, engine::ecs::EntityId entity) {
  const auto &healths = registry.GetComponents<HealthComponent>();
  const std::size_t index = static_cast<std::size_t>(entity);
  if (index < healths.size() && healths[index].has_value()) {
    return healths[index]->max >= kBossHealthThreshold;
  }
  return false;
}

}  // namespace

AnimationFactory::AnimationFactory(const ArchetypeRegistry &archetypes)
    : archetypes_(archetypes) {}

void AnimationFactory::EnsureAnimation(engine::ecs::Registry &registry,
                                       engine::ecs::EntityId entity,
                                       std::uint16_t type_code) const {
  const auto kind = archetypes_.KindOf(type_code);
  ApplyAnimation(registry, entity, kind);
}

void AnimationFactory::ApplyAnimation(engine::ecs::Registry &registry,
                                      engine::ecs::EntityId entity,
                                      ArchetypeKind kind) const {
  auto &animations = registry.GetComponents<AnimationComponent>();
  const std::size_t index = static_cast<std::size_t>(entity);
  if (index < animations.size() && animations[index].has_value()) {
    return;
  }

  switch (kind) {
    case ArchetypeKind::kPlayer:
      animations[entity] = AnimationComponent(PlayerFrames(), 0.1f);
      break;
    case ArchetypeKind::kEnemy:
      if (IsBoss(registry, entity)) {
        animations[entity] = AnimationComponent(BossFrames(), 0.1f);
      } else {
        animations[entity] = AnimationComponent(EnemyFrames(), 0.1f);
      }
      break;
    case ArchetypeKind::kMissile:
      animations[entity] = AnimationComponent(MissileFrames(), 0.1f);
      break;
    case ArchetypeKind::kObstacle:
    case ArchetypeKind::kPowerup:
    case ArchetypeKind::kUnknown:
    default:
      break;
  }
}

}  // namespace client::ecs
