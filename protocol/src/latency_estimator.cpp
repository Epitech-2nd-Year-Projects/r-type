#include "protocol/latency_estimator.h"

namespace protocol {

void LatencyEstimator::OnPingSent(std::uint32_t client_time_ms) {
  last_ping_client_time_ms_ = client_time_ms;
}

void LatencyEstimator::OnPongReceived(std::uint32_t client_time_ms,
                                      std::uint32_t server_time_ms,
                                      std::uint32_t now_ms) {
  std::uint32_t rtt_raw = 0;
  if (now_ms >= client_time_ms) {
    rtt_raw = now_ms - client_time_ms;
  } else {
    rtt_raw = 0;
  }

  last_rtt_ms_ = static_cast<float>(rtt_raw);
  const float half_rtt = last_rtt_ms_ * 0.5f;
  const float estimated_client_at_server_send =
      static_cast<float>(client_time_ms) + half_rtt;
  last_offset_ms_ =
      static_cast<float>(server_time_ms) - estimated_client_at_server_send;
  has_estimate_ = true;
  last_ping_client_time_ms_ = client_time_ms;
}
}  // namespace protocol
