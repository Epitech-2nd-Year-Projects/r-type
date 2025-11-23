#include "ecs.h"

void ECSGameWorld::AddSystem(std::function<void(ECSGameWorld &)> system) {
  systems_.push_back(system);
}

void ECSGameWorld::Update(float dt) {
  for (auto &system : systems_) system(*this);
}

size_t ECSGameWorld::EntityCount() const {
  return positions.size() < velocities.size() ? positions.size()
                                              : velocities.size();
}
