#ifndef PROTOCOL_LATENCY_ESTIMATOR_H_
#define PROTOCOL_LATENCY_ESTIMATOR_H_

#include <cstdint>

namespace protocol {

/**
 * @brief Helper class to estimate RTT and clock offset using Ping/Pong messages.
 * 
 * Protocol convention:
 *  - Client sends PingPayload{ client_time_ms = T_c_send }
 *  - Server responds with PongPayload{
 *        client_time_ms = T_c_send (echoed),
 *        server_time_ms = T_s_send_pong
 *    }
 *  - Client receives the Pong at T_c_recv (now_ms on client side)
 * 
 * Estimation formulas:
 *  - RTT ≈ T_c_recv - T_c_send
 *  - offset ≈ server_time_ms - (T_c_send + RTT / 2)
 *    (clock offset server - client, in ms, assuming symmetric latency)
 */
class LatencyEstimator {
 public:
  LatencyEstimator() = default;

  /**
   * @brief Called when a Ping is sent by this side.
   * @param client_time_ms The timestamp that will be written into PingPayload.
   * 
   * Records the ping send time for later RTT calculation when the pong is received.
   */
  void OnPingSent(std::uint32_t client_time_ms);

  /**
   * @brief Called when a Pong is received by this side.
   * @param client_time_ms Echoed client timestamp from PongPayload.
   * @param server_time_ms Server timestamp from PongPayload.
   * @param now_ms Local time when the Pong is received.
   * 
   * Updates the internal RTT and clock offset estimates based on the ping-pong cycle.
   */
  void OnPongReceived(std::uint32_t client_time_ms,
                      std::uint32_t server_time_ms,
                      std::uint32_t now_ms);

  /**
   * @brief Checks if at least one Ping/Pong cycle has been processed.
   * @return true if estimates are available, false otherwise.
   */
  bool has_estimate() const { return has_estimate_; }

  /**
   * @brief Returns the last measured round-trip time.
   * @return RTT in milliseconds.
   */
  float rtt_ms() const { return last_rtt_ms_; }

  /**
   * @brief Returns the last estimated clock offset (server - client).
   * @return Clock offset in milliseconds. Positive means server clock is ahead of client.
   */
  float clock_offset_ms() const { return last_offset_ms_; }

 private:
  bool has_estimate_{false};                   ///< Whether at least one estimate has been computed.
  std::uint32_t last_ping_client_time_ms_{0}; ///< Timestamp of the last ping sent.
  float last_rtt_ms_{0.0f};                   ///< Last computed round-trip time in ms.
  float last_offset_ms_{0.0f};                ///< Last computed clock offset (server - client) in ms.
};

}  // namespace protocol

#endif  // PROTOCOL_LATENCY_ESTIMATOR_H_
