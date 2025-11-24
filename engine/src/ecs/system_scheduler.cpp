#include "engine/ecs/system_scheduler.h"

#include <algorithm>

#include "engine/ecs/registry.h"

namespace engine::ecs {

SystemScheduler::SystemScheduler(time::TimeDelta fixed_timestep)
    : fixed_timestep_(fixed_timestep),
      accumulator_(time::TimeDelta::zero()),
      needs_sort_(false) {}

void SystemScheduler::RegisterSystem(
    std::function<void(Registry&, time::TimeDelta)> system_fn, SystemType type,
    SystemPriority priority) {
  systems_.emplace_back(std::move(system_fn), type, priority);
  needs_sort_ = true;
}

void SystemScheduler::Update(Registry& registry, time::TimeDelta dt) {
  if (needs_sort_) {
    SortSystems();
    needs_sort_ = false;
  }
  for (auto& entry : systems_) {
    if (entry.type == SystemType::Variable) {
      entry.function(registry, dt);
    }
  }
  accumulator_ += dt;

  constexpr int kMaxFixedIterations = 10;
  int iterations = 0;

  const time::TimeDelta max_accumulator =
      fixed_timestep_ * static_cast<float>(kMaxFixedIterations);
  if (accumulator_ > max_accumulator) {
    accumulator_ = max_accumulator;
  }

  while (accumulator_ >= fixed_timestep_ && iterations < kMaxFixedIterations) {
    for (auto& entry : systems_) {
      if (entry.type == SystemType::Fixed) {
        entry.function(registry, fixed_timestep_);
      }
    }

    accumulator_ -= fixed_timestep_;
    iterations++;
  }
}

void SystemScheduler::SetFixedTimestep(time::TimeDelta timestep) {
  fixed_timestep_ = timestep;
  accumulator_ = time::TimeDelta::zero();
}

time::TimeDelta SystemScheduler::FixedTimestep() const {
  return fixed_timestep_;
}

void SystemScheduler::Clear() {
  systems_.clear();
  accumulator_ = time::TimeDelta::zero();
  needs_sort_ = false;
}

void SystemScheduler::SortSystems() {
  std::sort(systems_.begin(), systems_.end(),
            [](const SystemEntry& a, const SystemEntry& b) {
              return a.priority > b.priority;
            });
}

}  // namespace engine::ecs