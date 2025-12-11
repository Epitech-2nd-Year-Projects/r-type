#include "ecs/interpolation_system.h"

#include <algorithm>

#include "engine/time/monotonic_time.h"

namespace client::ecs {

namespace {

engine::math::Vector2f Lerp(const engine::math::Vector2f& a,
                            const engine::math::Vector2f& b, float t) {
  return engine::math::Vector2f::Lerp(a, b, t);
}

}  // namespace

InterpolationSystem::InterpolationSystem(engine::ecs::Registry& registry)
    : registry_(registry) {
  RegisterComponents();
}

void InterpolationSystem::RegisterComponents() {
  registry_.RegisterComponent<PositionComponent>();
  registry_.RegisterComponent<VelocityComponent>();
  registry_.RegisterComponent<SnapshotInterpolationComponent>();
  registry_.RegisterComponent<LocalPlayerTag>();
}

bool InterpolationSystem::HasLocalPlayerTag(
    const engine::ecs::Registry& registry, engine::ecs::EntityId entity) {
  const auto& locals = registry.GetComponents<LocalPlayerTag>();
  const std::size_t idx = static_cast<std::size_t>(entity);
  return idx < locals.size() && locals[idx].has_value();
}

void InterpolationSystem::Update(engine::time::TimeDelta dt) {
  UpdateAt(dt, engine::time::NowMilliseconds());
}

void InterpolationSystem::UpdateAt(engine::time::TimeDelta /*dt*/,
                                   std::uint64_t now_ms) {
  auto& positions = registry_.GetComponents<PositionComponent>();
  const auto& velocities = registry_.GetComponents<VelocityComponent>();
  const auto& snapshots =
      registry_.GetComponents<SnapshotInterpolationComponent>();

  const std::size_t count = positions.size();
  const std::uint64_t render_time_ms =
      now_ms > interpolation_delay_ms_ ? now_ms - interpolation_delay_ms_ : 0u;

  for (std::size_t i = 0; i < count; ++i) {
    if (!positions[i].has_value()) {
      continue;
    }

    const auto entity = registry_.EntityFromIndex(i);
    if (HasLocalPlayerTag(registry_, entity)) {
      positions[i]->render_position = positions[i]->position;
      continue;
    }

    if (i >= snapshots.size() || !snapshots[i].has_value()) {
      positions[i]->render_position = positions[i]->position;
      continue;
    }

    const auto& timing = snapshots[i].value();
    if (timing.last_snapshot_ms == 0) {
      positions[i]->render_position = positions[i]->position;
      continue;
    }

    const std::uint64_t start_ms = timing.previous_snapshot_ms == 0
                                       ? timing.last_snapshot_ms
                                       : timing.previous_snapshot_ms;
    const std::uint64_t end_ms = timing.last_snapshot_ms;
    const float interval_ms =
        static_cast<float>(end_ms > start_ms ? end_ms - start_ms : 0);
    if (interval_ms <= 0.0f) {
      positions[i]->render_position = positions[i]->position;
      continue;
    }

    const float render_ms = static_cast<float>(
        render_time_ms > start_ms ? render_time_ms - start_ms : 0);
    float alpha = render_ms / interval_ms;
    alpha = std::max(0.0f, alpha);

    if (alpha <= 1.0f) {
      positions[i]->render_position =
          Lerp(positions[i]->previous_position, positions[i]->position, alpha);
      continue;
    }

    const std::uint64_t extra_ms =
        render_time_ms > end_ms ? render_time_ms - end_ms : 0;
    const std::uint64_t clamped_extra =
        std::min<std::uint64_t>(extra_ms, max_extrapolation_ms_);
    const float extra_seconds =
        static_cast<float>(clamped_extra) / 1000.0f;
    engine::math::Vector2f velocity{0.0f, 0.0f};
    if (i < velocities.size() && velocities[i].has_value()) {
      velocity = velocities[i]->velocity;
    }
    positions[i]->render_position =
        positions[i]->position + velocity * extra_seconds;
  }
}

}  // namespace client::ecs
