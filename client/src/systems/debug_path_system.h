#ifndef CLIENT_SYSTEMS_DEBUG_PATH_SYSTEM_H_
#define CLIENT_SYSTEMS_DEBUG_PATH_SYSTEM_H_

#include <unordered_map>

#include "ecs/render_debug.h"
#include "engine/ecs/registry.h"
#include "engine/ecs/system.h"

namespace client::systems {

class DebugPathSystem {
 public:
  DebugPathSystem(engine::ecs::Registry& registry,
                  client::ecs::RenderDebug& render_debug);

  void Update(engine::time::TimeDelta dt);

 private:
  engine::ecs::Registry& registry_;
  client::ecs::RenderDebug& render_debug_;
  float total_time_{0.0f};
  struct EntityState {
    float last_vy = 0.0f;
    float phase = 0.0f;
    bool initialized = false;
  };
  std::unordered_map<std::size_t, EntityState> entity_states_;
};

}  // namespace client::systems

#endif  // CLIENT_SYSTEMS_DEBUG_PATH_SYSTEM_H_
