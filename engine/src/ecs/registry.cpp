#include "engine/ecs/registry.h"

#include "engine/ecs/system_scheduler.h"

namespace engine::ecs {

Registry::Registry(time::TimeDelta fixed_timestep)
    : next_entity_id_(0),
      scheduler_(std::make_unique<SystemScheduler>(fixed_timestep)) {}

Registry::~Registry() = default;

void Registry::UpdateSystems(time::TimeDelta dt) {
  scheduler_->Update(*this, dt);
}

void Registry::SetFixedTimestep(time::TimeDelta timestep) {
  scheduler_->SetFixedTimestep(timestep);
}

time::TimeDelta Registry::FixedTimestep() const {
  return scheduler_->FixedTimestep();
}

void Registry::ClearSystems() {
  scheduler_->Clear();
  owned_systems_.clear();
}

void Registry::RunSystems() { UpdateSystems(time::TimeDelta::zero()); }

}  // namespace engine::ecs