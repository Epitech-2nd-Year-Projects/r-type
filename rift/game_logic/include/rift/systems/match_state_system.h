#ifndef RIFT_SYSTEMS_MATCH_STATE_SYSTEM_H_
#define RIFT_SYSTEMS_MATCH_STATE_SYSTEM_H_

#include <functional>

#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"
#include "engine/time/time_delta.h"

namespace rift {
class GameInstance;
}

namespace rift::systems {

class MatchStateSystem : public engine::ecs::ISystem {
 public:
  static constexpr std::uint32_t kRoundTimeLimitMs = 60000;
  static constexpr std::uint8_t kRoundsToWin = 2;

  explicit MatchStateSystem(GameInstance& instance);
  ~MatchStateSystem() override = default;

  void Update(engine::ecs::Registry& registry,
              engine::time::TimeDelta dt) override;

 private:
  GameInstance& instance_;
};

}  // namespace rift::systems

#endif  // RIFT_SYSTEMS_MATCH_STATE_SYSTEM_H_
