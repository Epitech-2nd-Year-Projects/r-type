#include "rift/systems/attack_system.h"

#include "engine/ecs/indexed_zipper.h"
#include "rift/components/fighter_component.h"

namespace rift::systems {

void AttackSystem::Update(engine::ecs::Registry& registry,
                          engine::time::TimeDelta dt) {
  auto& attacks = registry.GetComponents<components::AttackComponent>();
  auto& hitboxes = registry.GetComponents<components::HitboxComponent>();

  accumulated_time_ms_ += static_cast<std::uint32_t>(dt.as_milliseconds());

  while (accumulated_time_ms_ >= kFrameTimeMs) {
    accumulated_time_ms_ -= kFrameTimeMs;

    for (auto [idx, attack] : engine::ecs::IndexedZipper(attacks)) {
      if (attack->type == components::AttackType::kNone) continue;

      attack->current_frame++;

      if (attack->IsComplete()) {
        attack->Reset();
      }
    }
  }

  for (auto [idx, attack, hitbox] :
       engine::ecs::IndexedZipper(attacks, hitboxes)) {
    hitbox->active = attack->IsActive() && !attack->hit_confirmed;
  }
}

}  // namespace rift::systems
