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
#include <string>
#include <string_view>
#include <vector>

#include "engine/input.h"
#include "engine/math/rect.h"
#include "engine/math/vector2.h"
#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"
#include "engine/ui/button.h"
#include "protocol/lobby.h"
#include "ui/menu_effects.h"

namespace client {

class ClientAssetManager;
class ClientContext;

/**
 * @brief View for the lobby room list
 */
class LobbyRoomListView {
 public:
  /**
   * @brief Create the room list view
   * @param context Client context reference
   * @param on_room_selected Callback for room selection
   */
  explicit LobbyRoomListView(
      ClientContext& context,
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
   * @brief Draw lobby background
   */
  void DrawBackground() const;

  /**
   * @brief Draw lobby foreground
   * @param renderer Renderer instance
   * @param status_text Directory status text
   */
  void DrawForeground(engine::render::Renderer2D& renderer,
                      std::string_view status_text) const;

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
  void ApplyRoomLayout();
  void UpdateScrollInput(engine::input::InputManager& input);
  void UpdateScrollHandleRect();
  void DrawScrollBar(engine::render::Renderer2D& renderer) const;

  struct RoomEntry {
    protocol::RoomSummary room{};
    std::string left_text;
    std::string center_text;
    std::string right_text;
    std::shared_ptr<engine::render::Texture2D> area_texture;
  };

  ClientContext& context_;
  std::function<void(const protocol::RoomSummary&)> on_room_selected_;
  ui::MenuEffects menu_effects_;
  std::vector<std::shared_ptr<engine::ui::Button>> room_buttons_;
  std::vector<std::shared_ptr<engine::ui::Button>> room_buttons_visible_;
  std::vector<RoomEntry> room_entries_;
  std::vector<std::shared_ptr<engine::render::Texture2D>> area_textures_;
  std::vector<std::shared_ptr<engine::render::Texture2D>> header_frames_;
  std::vector<std::shared_ptr<engine::render::Texture2D>> room_frames_;
  std::shared_ptr<engine::render::Texture2D> scroll_handle_texture_;
  std::shared_ptr<engine::render::Texture2D> scroll_track_end_texture_;
  std::shared_ptr<engine::render::Texture2D> scroll_track_mid_texture_;
  engine::math::RectF header_frame_rect_{};
  engine::math::Vector2f title_anchor_{};
  engine::math::Vector2f room_frame_draw_size_{};
  engine::math::Vector2f room_list_origin_{};
  engine::math::RectF room_list_viewport_{};
  engine::math::RectF scroll_track_rect_{};
  engine::math::RectF scroll_handle_rect_{};
  std::size_t last_rooms_hash_{0};
  float header_elapsed_{0.0f};
  bool header_animating_{true};
  float room_frame_elapsed_{0.0f};
  std::size_t room_frame_index_{0};
  bool room_frame_animating_{true};
  float room_list_content_height_{0.0f};
  float scroll_offset_{0.0f};
  float scroll_max_offset_{0.0f};
  float scroll_step_{0.0f};
  float scroll_track_scale_{1.0f};
  float scroll_handle_height_{0.0f};
  float scroll_drag_offset_{0.0f};
  float scroll_wheel_accumulator_{0.0f};
  bool scroll_dragging_{false};
  bool was_left_down_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_LOBBY_ROOM_LIST_VIEW_H_
