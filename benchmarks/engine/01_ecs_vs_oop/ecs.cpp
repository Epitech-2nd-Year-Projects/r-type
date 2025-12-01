#include "ecs.h"

void ECSGameWorld::AddSystem(std::function<void(ECSGameWorld &)> system) {
  systems_.push_back(system);
}

void ECSGameWorld::Update(float /*dt*/) {
  for (auto &system : systems_) system(*this);
}

void ECSGameWorld::SerializeAll(std::vector<SerializedEntity> &out) const {
  const size_t count =
      positions.size() < velocities.size() ? positions.size() : velocities.size();

  out.clear();
  out.reserve(count);

  for (size_t i = 0; i < count; ++i) {
    if (!positions[i].has_value() || !velocities[i].has_value()) continue;

    SerializedEntity entity{
        positions[i]->x,
        positions[i]->y,
        velocities[i]->vx,
        velocities[i]->vy,
        (i < healths.size() && healths[i].has_value()) ? healths[i]->hp : 0,
    };
    out.push_back(entity);
  }
}

size_t ECSGameWorld::EntityCount() const {
  size_t count = positions.size() < velocities.size() ? positions.size()
                                                      : velocities.size();
  if (healths.size() < count) count = healths.size();
  return count;
}
