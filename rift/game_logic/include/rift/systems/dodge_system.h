#ifndef RIFT_SYSTEMS_DODGE_SYSTEM_H_
#define RIFT_SYSTEMS_DODGE_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace rift::systems {

class DodgeSystem : public engine::ecs::ISystem {
 public:
  DodgeSystem() = default;
  ~DodgeSystem() override = default;

  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;
};

}  // namespace rift::systems

#endif  // RIFT_SYSTEMS_DODGE_SYSTEM_H_
