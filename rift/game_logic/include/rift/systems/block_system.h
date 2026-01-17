#ifndef RIFT_SYSTEMS_BLOCK_SYSTEM_H_
#define RIFT_SYSTEMS_BLOCK_SYSTEM_H_

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace rift::systems {

class BlockSystem : public engine::ecs::ISystem {
 public:
  BlockSystem() = default;
  ~BlockSystem() override = default;

  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;
};

}  // namespace rift::systems

#endif  // RIFT_SYSTEMS_BLOCK_SYSTEM_H_
