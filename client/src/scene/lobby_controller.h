/**
 * @file lobby_controller_h
 * @brief Lobby input and action controller
 *
 * @details
 * Owns lobby controls and banner state and triggers network actions
 */

#ifndef CLIENT_SCENE_LOBBY_CONTROLLER_H_
#define CLIENT_SCENE_LOBBY_CONTROLLER_H_

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "engine/input.h"
#include "engine/math/vector2.h"
#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"
#include "engine/ui/button.h"
#include "engine/ui/text_input.h"
#include "engine/ui/widget.h"
#include "protocol/lobby.h"

namespace client {

class ClientContext;

/**
 * @brief Lobby controller handling inputs and actions
 */
class LobbyController {
 public:
  /**
   * @brief Create a lobby controller
   * @param context Client context reference
   * @param open_create_modal Callback to open create modal
   */
  LobbyController(ClientContext& context,
                  std::function<void()> open_create_modal);

  /**
   * @brief Update control state
   * @param dt Frame time delta
   * @param input Input manager reference
   */
  void Update(engine::time::TimeDelta dt,
              engine::input::InputManager& input);

  /**
   * @brief Layout controls using the current viewport
   * @param window_size Window size in pixels
   */
  void Layout(const engine::math::Vector2f& window_size);

  /**
   * @brief Draw controls and banner
   * @param renderer Renderer instance
   */
  void Draw(engine::render::Renderer2D& renderer) const;

  /**
   * @brief Apply button visuals for refresh and create actions
   * @param renderer Renderer instance
   */
  void ApplyButtonStyle(engine::render::Renderer2D& renderer);

  /**
   * @brief Set focus based on pointer location
   * @param input Input manager reference
   */
  void HandleFocus(const engine::input::InputManager& input);

  /**
   * @brief Check if any input field is focused
   * @return True when an input field is focused
   */
  bool IsInputCaptured() const;

  /**
   * @brief Attempt to create a room
   * @param room_name Room name
   * @param max_players_text Raw max players input
   * @param is_private Privacy flag
   * @param password Room password
   * @return True when the request is accepted
   */
  bool TryCreateRoom(const std::string& room_name,
                     const std::string& max_players_text, bool is_private,
                     std::string password);

  /**
   * @brief Attempt to join a room
   * @param room Room summary
   * @param password Room password
   * @return True when the request is accepted
   */
  bool TryJoinRoom(const protocol::RoomSummary& room,
                   const std::string& password);

 private:
  void RefreshRoomList();
  void UpdateBannerFromCreation();
  void SetBanner(std::string message);

  ClientContext& context_;
  std::function<void()> open_create_modal_;

  std::vector<std::shared_ptr<engine::ui::Widget>> controls_;
  std::shared_ptr<engine::ui::TextInput> host_input_;
  std::shared_ptr<engine::ui::TextInput> port_input_;
  std::shared_ptr<engine::ui::TextInput> name_input_;
  std::shared_ptr<engine::ui::Button> refresh_button_;
  std::shared_ptr<engine::ui::Button> create_button_;

  engine::math::Vector2f host_label_pos_{};
  engine::math::Vector2f port_label_pos_{};
  engine::math::Vector2f name_label_pos_{};
  engine::math::Vector2f banner_pos_{};

  std::string lobby_host_;
  std::uint16_t lobby_port_{0};

  std::string banner_text_;
  std::chrono::steady_clock::time_point banner_expiry_{};
};

}  // namespace client

#endif  // CLIENT_SCENE_LOBBY_CONTROLLER_H_
