/**
 * @file hud_overlay.h
 * @brief Heads-up display for gameplay state
 */

#ifndef CLIENT_HUD_OVERLAY_H_
#define CLIENT_HUD_OVERLAY_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "engine/math/vector2.h"

namespace engine::ecs {
class Registry;
}  // namespace engine::ecs

namespace engine::render {
class Renderer2D;
}  // namespace engine::render

namespace client {

/**
 * @brief Scoreboard row describing a single player
 */
struct HudPlayerRow {
  std::uint32_t player_id{0};
  std::uint32_t score{0};
  std::uint32_t lives{0};
  bool is_local{false};
  bool alive{false};
};

/**
 * @class HudOverlay
 * @brief Minimal in-game HUD showing scores, lives and connection status
 */
class HudOverlay {
 public:
  HudOverlay() = default;

  /**
   * @brief Refresh player rows from the current ECS state
   * @param registry Registry containing networked entities and health
   * @param local_player_id Player id assigned to this client
   */
  void UpdatePlayers(const engine::ecs::Registry& registry,
                     std::optional<std::uint32_t> local_player_id);

  /**
   * @brief Update wave and level information for display
   * @param level Current level number
   * @param wave Current wave number
   */
  void UpdateWaveAndLevel(std::optional<std::uint32_t> level,
                          std::optional<std::uint32_t> wave);

  /**
   * @brief Update connection indicators
   * @param latency_ms Latest round trip time in milliseconds
   * @param connected Whether the transport is active
   * @param status_text Human readable connection status
   */
  void UpdateNetwork(std::optional<float> latency_ms, bool connected,
                     std::string status_text);

  /**
   * @brief Draw the HUD overlay
   * @param renderer Renderer used for text and primitives
   * @param window_size Current window dimensions
   */
  void Draw(engine::render::Renderer2D& renderer,
            const engine::math::Vector2i& window_size) const;

  /**
   * @brief Access the current player rows
   */
  const std::vector<HudPlayerRow>& players() const { return players_; }

  /**
   * @brief Current level when available
   */
  std::optional<std::uint32_t> level() const { return current_level_; }

  /**
   * @brief Current wave when available
   */
  std::optional<std::uint32_t> wave() const { return current_wave_; }

  /**
   * @brief Latest latency measurement
   */
  std::optional<float> latency_ms() const { return latency_ms_; }

  /**
   * @brief Connection state flag
   */
  bool connected() const { return connected_; }

  /**
   * @brief Human readable connection message
   */
  const std::string& status_text() const { return status_text_; }

 private:
  std::vector<HudPlayerRow> players_;
  std::optional<std::uint32_t> current_level_{1u};
  std::optional<std::uint32_t> current_wave_{1u};
  std::optional<float> latency_ms_{};
  bool connected_{false};
  std::string status_text_{};
};

}  // namespace client

#endif  // CLIENT_HUD_OVERLAY_H_
