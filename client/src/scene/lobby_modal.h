/**
 * @file lobby_modal_h
 * @brief Lobby modal dialog view
 *
 * @details
 * Handles create and join modal content and input
 */

#ifndef CLIENT_SCENE_LOBBY_MODAL_H_
#define CLIENT_SCENE_LOBBY_MODAL_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "engine/input.h"
#include "engine/math/rect.h"
#include "engine/math/vector2.h"
#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"
#include "engine/ui/button.h"
#include "engine/ui/text_input.h"
#include "engine/ui/widget.h"
#include "protocol/lobby.h"

namespace client {

/**
 * @brief Modal dialog for lobby actions
 */
class LobbyModal {
 public:
  /**
   * @brief Create a lobby modal
   * @param on_create Callback for create requests
   * @param on_join Callback for join requests
   */
  LobbyModal(
      std::function<bool(const std::string& room_name,
                         const std::string& max_players_text, bool is_private,
                         std::string password, protocol::Difficulty difficulty)>
          on_create,
      std::function<bool(const protocol::RoomSummary& room,
                         const std::string& password)>
          on_join);

  /**
   * @brief Update modal widgets
   * @param dt Frame time delta
   * @param input Input manager reference
   */
  void Update(engine::time::TimeDelta dt, engine::input::InputManager& input);

  /**
   * @brief Layout modal widgets
   * @param window_size Window size in pixels
   */
  void Layout(const engine::math::Vector2f& window_size);

  /**
   * @brief Draw modal content
   * @param renderer Renderer instance
   */
  void Draw(engine::render::Renderer2D& renderer) const;

  /**
   * @brief Apply focus based on pointer location
   * @param input Input manager reference
   */
  void HandleFocus(const engine::input::InputManager& input);

  /**
   * @brief Open the create modal
   */
  void OpenCreate();

  /**
   * @brief Open the join modal
   * @param room Room to join
   */
  void OpenJoin(const protocol::RoomSummary& room);

  /**
   * @brief Close the modal
   */
  void Close();

  /**
   * @brief Check if the modal is visible
   * @return True when open
   */
  bool IsOpen() const { return show_modal_; }

  /**
   * @brief Check if any modal input is focused
   * @return True when an input field is focused
   */
  bool IsInputCaptured() const;

 private:
  enum class ModalMode { kCreate, kJoinPrivate };

  void BuildModal();
  void ApplyPrimaryAction();
  std::vector<std::shared_ptr<engine::ui::Widget>> ActiveElements() const;

  std::function<bool(const std::string& room_name,
                     const std::string& max_players_text, bool is_private,
                     std::string password, protocol::Difficulty difficulty)>
      on_create_;
  std::function<bool(const protocol::RoomSummary& room,
                     const std::string& password)>
      on_join_;

  std::shared_ptr<engine::ui::TextInput> room_name_input_;
  std::shared_ptr<engine::ui::TextInput> max_players_input_;
  std::shared_ptr<engine::ui::Button> privacy_button_;
  std::shared_ptr<engine::ui::Button> difficulty_button_;
  std::shared_ptr<engine::ui::TextInput> password_input_;
  std::shared_ptr<engine::ui::Button> primary_button_;
  std::shared_ptr<engine::ui::Button> cancel_button_;

  std::vector<std::shared_ptr<engine::ui::Widget>> create_elements_;
  std::vector<std::shared_ptr<engine::ui::Widget>> join_elements_;

  engine::math::Vector2f viewport_size_{};
  engine::math::RectF modal_rect_{};
  bool show_modal_{false};
  bool modal_private_{false};
  ModalMode modal_mode_{ModalMode::kCreate};
  protocol::Difficulty modal_difficulty_{protocol::Difficulty::kNormal};
  std::string pending_join_room_code_;
  std::string pending_join_room_name_;
};

}  // namespace client

#endif  // CLIENT_SCENE_LOBBY_MODAL_H_
