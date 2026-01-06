#ifndef ENGINE_DEBUG_NETWORK_DEBUGGER_H_
#define ENGINE_DEBUG_NETWORK_DEBUGGER_H_

#include <chrono>
#include <random>

namespace engine::debug {

struct NetworkConditions {
  float packet_loss_percent{0.0f};
  float latency_ms{0.0f};
  float latency_variance_ms{0.0f};
  bool enabled{false};
};

class NetworkDebugger {
 public:
  NetworkDebugger();

  void DrawPanel();
  bool ShouldDropPacket();
  std::chrono::milliseconds GetDelayDuration();

  NetworkConditions& conditions() { return conditions_; }

 private:
  NetworkConditions conditions_;
  std::mt19937 rng_{std::random_device{}()};
};

}  // namespace engine::debug

#endif  // ENGINE_DEBUG_NETWORK_DEBUGGER_H_
