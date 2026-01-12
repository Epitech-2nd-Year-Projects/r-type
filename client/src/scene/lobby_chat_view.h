/**
 * @file lobby_chat_view.h
 * @brief Lobby chat panel UI component
 *
 * @details
 * Displays chat messages and provides text input for composing new messages
 */

#ifndef CLIENT_SCENE_LOBBY_CHAT_VIEW_H_
#define CLIENT_SCENE_LOBBY_CHAT_VIEW_H_

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <string>

#include "engine/input.h"
#include "engine/math/rect.h"
#include "engine/math/vector2.h"
#include "engine/render/renderer2d.h"
#include "engine/time/time_delta.h"
#include "engine/ui/button.h"
#include "engine/ui/text_input.h"
#include "lobby_chat_service.h"

namespace client {

class ClientAssetManager;
class ClientContext;

/**
 * @brief Chat panel view for lobby communication
 */
class LobbyChatView {
 public:
  /// Maximum visible messages in the panel.
  static constexpr std::size_t kMaxVisibleMessages = 20;

  using SendCallback = std::function<bool(std::string_view)>;

  /**
   * @brief Create the chat view
   * @param context Client context reference
   * @param on_send Callback for sending messages
   */
  explicit LobbyChatView(ClientContext& context, SendCallback on_send);

  /**
   * @brief Update chat view state
   * @param dt Frame time delta
   * @param input Input manager reference
   */
  void Update(engine::time::TimeDelta dt, engine::input::InputManager& input);

  /**
   * @brief Layout the chat panel
   * @param panel_rect Available rectangle for the chat panel
   */
  void Layout(const engine::math::RectF& panel_rect);

  /**
   * @brief Draw the chat panel
   * @param renderer Renderer instance
   */
  void Draw(engine::render::Renderer2D& renderer) const;

  /**
   * @brief Apply button and input styles
   * @param assets Asset manager reference
   */
  void ApplyStyle(ClientAssetManager& assets);

  /**
   * @brief Add a message to the display
   * @param message Chat message to display
   */
  void AddMessage(const ChatMessage& message);

  /**
   * @brief Sync messages from the chat service
   * @param messages Message history
   */
  void SyncMessages(const std::deque<ChatMessage>& messages);

  /**
   * @brief Clear all displayed messages
   */
  void ClearMessages();

  /**
   * @brief Check if the chat input is focused
   * @return True when text input has focus
   */
  bool IsInputCaptured() const;

  /**
   * @brief Set focus on message input field
   * @param focused Focus state
   */
  void SetInputFocused(bool focused);

 private:
  void UpdateInputFocus(engine::input::InputManager& input);
  void HandleSendAction();

  ClientContext& context_;
  SendCallback on_send_;
  engine::math::RectF panel_rect_{};
  engine::math::RectF messages_rect_{};
  engine::math::RectF input_rect_{};
  engine::math::RectF send_button_rect_{};

  std::shared_ptr<engine::ui::TextInput> message_input_;
  std::shared_ptr<engine::ui::Button> send_button_;

  std::deque<std::string> display_messages_;
  float scroll_offset_{0.0f};
  bool enter_pressed_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_LOBBY_CHAT_VIEW_H_
