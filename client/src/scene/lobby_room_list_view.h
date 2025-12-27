/**
 * @file lobby_room_list_view_h
 * @brief Lobby room list view
 *
 * @details
 * Displays room entries and handles selection input
 */

#ifndef CLIENT_SCENE_LOBBY_ROOM_LIST_VIEW_H_
#define CLIENT_SCENE_LOBBY_ROOM_LIST_VIEW_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "engine/input.h"
#include "engine/math/vector2.h"
#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"
#include "engine/ui/button.h"
#include "protocol/lobby.h"

namespace client {

class ClientAssetManager;

/**
 * @brief View for the lobby room list
 */
class LobbyRoomListView {
 public:
  /**
   * @brief Create the room list view
   * @param on_room_selected Callback for room selection
   */
  explicit LobbyRoomListView(
      std::function<void(const protocol::RoomSummary&)> on_room_selected);

  /**
   * @brief Update button states
   * @param dt Frame time delta
   * @param input Input manager reference
   */
  void Update(engine::time::TimeDelta dt,
              engine::input::InputManager& input);

  /**
   * @brief Layout room list elements
   * @param window_size Window size in pixels
   */
  void Layout(const engine::math::Vector2f& window_size);

  /**
   * @brief Draw room list and status
   * @param renderer Renderer instance
   * @param status_text Directory status text
   */
  void Draw(engine::render::Renderer2D& renderer,
            std::string_view status_text) const;

  /**
   * @brief Sync room entries with current data
   * @param rooms Room list
   * @param assets Asset manager reference
   */
  void RefreshRooms(const std::vector<protocol::RoomSummary>& rooms,
                    ClientAssetManager& assets);

 private:
  std::function<void(const protocol::RoomSummary&)> on_room_selected_;
  std::vector<std::shared_ptr<engine::ui::Button>> room_buttons_;
  std::shared_ptr<engine::render::Texture2D> button_texture_{};
  engine::math::Vector2f title_pos_{};
  engine::math::Vector2f status_pos_{};
  std::size_t last_rooms_hash_{0};
};

}  // namespace client

#endif  // CLIENT_SCENE_LOBBY_ROOM_LIST_VIEW_H_
