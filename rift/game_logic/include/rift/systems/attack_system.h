#ifndef RIFT_SYSTEMS_ATTACK_SYSTEM_H_
#define RIFT_SYSTEMS_ATTACK_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace rift::systems {

class AttackSystem : public engine::ecs::ISystem {
 public:
  static constexpr std::uint32_t kFrameTimeMs = 16;

  AttackSystem() = default;
  ~AttackSystem() override = default;

  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;

 private:
  std::uint32_t accumulated_time_ms_{0};
};

}  // namespace rift::systems

#endif  // RIFT_SYSTEMS_ATTACK_SYSTEM_H_
