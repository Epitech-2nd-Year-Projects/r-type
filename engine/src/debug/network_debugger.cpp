#include "engine/debug/network_debugger.h"

#include <imgui.h>

namespace engine::debug {

NetworkDebugger::NetworkDebugger() = default;

void NetworkDebugger::DrawPanel() {
  ImGui::Begin("Network Debugger");
  ImGui::Checkbox("Enable Simulation", &conditions_.enabled);

  if (conditions_.enabled) {
    ImGui::SliderFloat("Packet Loss (%)", &conditions_.packet_loss_percent,
                       0.0f, 100.0f);
    ImGui::SliderFloat("Latency (ms)", &conditions_.latency_ms, 0.0f, 1000.0f);
    ImGui::SliderFloat("Jitter (ms)", &conditions_.latency_variance_ms, 0.0f,
                       500.0f);
  } else {
    ImGui::TextDisabled("Simulation disabled");
  }
  ImGui::End();
}

bool NetworkDebugger::ShouldDropPacket() {
  if (!conditions_.enabled || conditions_.packet_loss_percent <= 0.0f) {
    return false;
  }
  std::uniform_real_distribution<float> dist(0.0f, 100.0f);
  return dist(rng_) < conditions_.packet_loss_percent;
}

std::chrono::milliseconds NetworkDebugger::GetDelayDuration() {
  if (!conditions_.enabled || conditions_.latency_ms <= 0.0f) {
    return std::chrono::milliseconds(0);
  }

  float delay = conditions_.latency_ms;
  if (conditions_.latency_variance_ms > 0.0f) {
    std::uniform_real_distribution<float> dist(-conditions_.latency_variance_ms,
                                               conditions_.latency_variance_ms);
    delay += dist(rng_);
  }

  if (delay < 0.0f) delay = 0.0f;

  return std::chrono::milliseconds(static_cast<long long>(delay));
}

}  // namespace engine::debug
