#ifndef PROTOCOL_LATENCY_ESTIMATOR_H_
#define PROTOCOL_LATENCY_ESTIMATOR_H_

#include <cstdint>

namespace protocol {

/**
 * @brief Helper to estimate RTT and clock offset using Ping/Pong.
 *
 * Convention:
 *  - Client envoie PingPayload{ client_time_ms = T_c_send }.
 *  - Serveur renvoie PongPayload{
 *        client_time_ms = T_c_send,
 *        server_time_ms = T_s_send_pong
 *    }.
 *  - Client reçoit le Pong à T_c_recv (now_ms côté client).
 *
 * On estime:
 *  - RTT ≈ T_c_recv - T_c_send
 *  - offset ≈ server_time_ms - (T_c_send + RTT / 2)
 *    (décalage d'horloge serveur - client, en ms, approx symétrique).
 */
class LatencyEstimator {
 public:
  LatencyEstimator() = default;

  /**
   * @brief Call when a Ping is sent by this side.
   *
   * @param client_time_ms The timestamp that will be written into PingPayload.
   */
  void OnPingSent(std::uint32_t client_time_ms);

  /**
   * @brief Call when a Pong is received by this side.
   *
   * @param client_time_ms  Echoed client timestamp from PongPayload.
   * @param server_time_ms  Server timestamp from PongPayload.
   * @param now_ms          Local time when the Pong is received.
   *
   * Updates the internal RTT and clock offset estimates.
   */
  void OnPongReceived(std::uint32_t client_time_ms,
                      std::uint32_t server_time_ms,
                      std::uint32_t now_ms);

  /**
   * @brief True if at least one Ping/Pong cycle has been processed.
   */
  bool has_estimate() const { return has_estimate_; }

  /**
   * @brief Last measured round-trip time in milliseconds.
   */
  float rtt_ms() const { return last_rtt_ms_; }

  /**
   * @brief Last estimated clock offset (server - client) in milliseconds.
   *
   * Positive value means server clock is ahead of client.
   */
  float clock_offset_ms() const { return last_offset_ms_; }

 private:
  bool has_estimate_{false};
  std::uint32_t last_ping_client_time_ms_{0};
  float last_rtt_ms_{0.0f};
  float last_offset_ms_{0.0f};
};

}  // namespace protocol

#endif  // PROTOCOL_LATENCY_ESTIMATOR_H_
