#ifndef RIFT_SYSTEMS_FIGHTER_INPUT_SYSTEM_H_
#define RIFT_SYSTEMS_FIGHTER_INPUT_SYSTEM_H_

#include <functional>

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace rift {
class GameInstance;
}

namespace rift::systems {

class FighterInputSystem : public engine::ecs::ISystem {
 public:
  explicit FighterInputSystem(GameInstance& instance);
  ~FighterInputSystem() override = default;

  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;

 private:
  GameInstance& instance_;
};

}  // namespace rift::systems

#endif  // RIFT_SYSTEMS_FIGHTER_INPUT_SYSTEM_H_
