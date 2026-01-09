#include "engine/debug/network_debugger.h"

#include <imgui.h>

namespace engine::debug {

NetworkDebugger::NetworkDebugger() = default;

void NetworkDebugger::DrawPanel() {
  ImGui::Begin("Network Debugger");

  bool enabled = conditions_.enabled.load();
  if (ImGui::Checkbox("Enable Simulation", &enabled)) {
    conditions_.enabled.store(enabled);
  }

  if (enabled) {
    float loss = conditions_.packet_loss_percent.load();
    if (ImGui::SliderFloat("Packet Loss (%)", &loss, 0.0f, 100.0f)) {
      conditions_.packet_loss_percent.store(loss);
    }

    float latency = conditions_.latency_ms.load();
    if (ImGui::SliderFloat("Latency (ms)", &latency, 0.0f, 1000.0f)) {
      conditions_.latency_ms.store(latency);
    }

    float jitter = conditions_.latency_variance_ms.load();
    if (ImGui::SliderFloat("Jitter (ms)", &jitter, 0.0f, 500.0f)) {
      conditions_.latency_variance_ms.store(jitter);
    }
  } else {
    ImGui::TextDisabled("Simulation disabled");
  }
  ImGui::End();
}

bool NetworkDebugger::ShouldDropPacket() {
  if (!conditions_.enabled.load() ||
      conditions_.packet_loss_percent.load() <= 0.0f) {
    return false;
  }
  std::uniform_real_distribution<float> dist(0.0f, 100.0f);
  return dist(rng_) < conditions_.packet_loss_percent.load();
}

std::chrono::milliseconds NetworkDebugger::GetDelayDuration() {
  if (!conditions_.enabled.load() || conditions_.latency_ms.load() <= 0.0f) {
    return std::chrono::milliseconds(0);
  }

  float delay = conditions_.latency_ms.load();
  float variance = conditions_.latency_variance_ms.load();

  if (variance > 0.0f) {
    std::uniform_real_distribution<float> dist(-variance, variance);
    delay += dist(rng_);
  }

  if (delay < 0.0f) delay = 0.0f;

  return std::chrono::milliseconds(static_cast<long long>(delay));
}

}  // namespace engine::debug
