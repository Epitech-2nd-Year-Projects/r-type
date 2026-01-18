/**
 * @file lobby_chat_view.h
 * @brief Lobby chat panel UI component
 *
 * @details
 * Displays chat messages and provides text input for composing new messages.
 * Supports minimize, resize, and drag functionality.
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

  /// Title bar height.
  static constexpr float kTitleBarHeight = 28.0f;

  /// Resize handle size.
  static constexpr float kResizeHandleSize = 12.0f;

  /// Minimum panel dimensions.
  static constexpr float kMinWidth = 200.0f;
  static constexpr float kMinHeight = 150.0f;

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
   * @brief Set the initial/default bounds for the chat panel
   * @param panel_rect Default rectangle for the chat panel  
   */
  void SetDefaultBounds(const engine::math::RectF& panel_rect);

  /**
   * @brief Layout the chat panel internals based on current bounds
   */
  void Layout();

  /**
   * @brief Legacy layout method for compatibility
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

  /**
   * @brief Check if the panel is minimized
   */
  bool IsMinimized() const { return minimized_; }

  /**
   * @brief Set minimized state
   */
  void SetMinimized(bool minimized) { minimized_ = minimized; }

  /**
   * @brief Toggle minimized state
   */
  void ToggleMinimized() { minimized_ = !minimized_; }

 private:
  void UpdateInputFocus(engine::input::InputManager& input);
  void UpdateDragging(engine::input::InputManager& input);
  void UpdateResizing(engine::input::InputManager& input);
  void HandleSendAction();
  void ClampToScreen();

  ClientContext& context_;
  SendCallback on_send_;

  // Panel geometry
  engine::math::RectF panel_rect_{};
  engine::math::RectF title_bar_rect_{};
  engine::math::RectF messages_rect_{};
  engine::math::RectF input_rect_{};
  engine::math::RectF send_button_rect_{};
  engine::math::RectF resize_handle_rect_{};
  engine::math::RectF minimize_button_rect_{};

  // UI elements
  std::shared_ptr<engine::ui::TextInput> message_input_;
  std::shared_ptr<engine::ui::Button> send_button_;

  // Display state
  std::deque<ChatMessage> display_messages_;
  float scroll_offset_{0.0f};
  bool enter_pressed_{false};

  // Interaction state
  bool minimized_{false};
  bool dragging_{false};
  bool resizing_{false};
  engine::math::Vector2f drag_offset_{};
  engine::math::Vector2f resize_start_size_{};
  engine::math::Vector2f resize_start_mouse_{};
  bool mouse_was_down_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_LOBBY_CHAT_VIEW_H_

