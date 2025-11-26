#include "engine/ecs/systems/lifetime_system.h"

#include <vector>

#include "engine/ecs/components/lifetime_component.h"
#include "engine/ecs/indexed_zipper.h"

namespace engine::ecs {

void LifetimeSystem::Update(Registry& registry,
                            SparseArray<LifetimeComponent>& lifetimes,
                            time::TimeDelta dt) {
  std::vector<EntityId> to_kill;

  for (auto&& [idx, lifetime] : IndexedZipper(lifetimes)) {
    lifetime.value().remaining -= dt;

    if (lifetime.value().remaining <= time::TimeDelta::zero()) {
      to_kill.push_back(registry.EntityFromIndex(idx));
    }
  }
  for (const auto& entity : to_kill) {
    registry.KillEntity(entity);
  }
}

}  // namespace engine::ecs