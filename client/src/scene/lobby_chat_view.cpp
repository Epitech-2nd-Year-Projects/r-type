/**
 * @file lobby_chat_view.cpp
 * @brief Implementation of lobby chat panel view
 */

#include "lobby_chat_view.h"

#include <algorithm>
#include <utility>

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/ui_constants.h"
#include "engine/render/color.h"
#include "protocol/chat.h"

namespace client {

namespace {
namespace ui_constants = constants::ui;
}  // namespace

LobbyChatView::LobbyChatView(ClientContext& context, SendCallback on_send)
    : context_(context),
      on_send_(std::move(on_send)),
      message_input_(std::make_shared<engine::ui::TextInput>(
          engine::math::Vector2f{0.0f, 0.0f},
          engine::math::Vector2f{200.0f, ui_constants::Lobby::kChatInputHeight})),
      send_button_(std::make_shared<engine::ui::Button>(
          engine::math::Vector2f{0.0f, 0.0f},
          engine::math::Vector2f{ui_constants::Lobby::kChatSendButtonWidth,
                                 ui_constants::Lobby::kChatSendButtonHeight},
          "Send", [this]() { HandleSendAction(); })) {
  message_input_->SetPlaceholder("Type a message...");
  message_input_->SetBackgroundColor(ui_constants::Lobby::kChatInputBgColor);
  message_input_->SetTextColor(engine::render::Color::White());
}

void LobbyChatView::Update(engine::time::TimeDelta dt,
                           engine::input::InputManager& input) {
  (void)dt;

  UpdateInputFocus(input);

  message_input_->Update(dt, input);
  send_button_->Update(dt, input);

  // Handle Enter key to send message
  const bool enter_down = input.IsKeyDown(engine::input::Key::kEnter);
  if (enter_down && !enter_pressed_ && message_input_->IsFocused()) {
    HandleSendAction();
  }
  enter_pressed_ = enter_down;
}

void LobbyChatView::Layout(const engine::math::RectF& panel_rect) {
  panel_rect_ = panel_rect;

  const float padding = ui_constants::Lobby::kChatPanelPadding;
  const float input_height = ui_constants::Lobby::kChatInputHeight;
  const float input_spacing = ui_constants::Lobby::kChatInputSpacing;
  const float send_width = ui_constants::Lobby::kChatSendButtonWidth;
  const float send_height = ui_constants::Lobby::kChatSendButtonHeight;

  // Input area at bottom
  input_rect_ = engine::math::RectF{
      panel_rect.top_left_x_ + padding,
      panel_rect.top_left_y_ + panel_rect.height_ - padding - input_height,
      panel_rect.width_ - padding * 2.0f - send_width - input_spacing,
      input_height};

  // Send button to the right of input
  send_button_rect_ = engine::math::RectF{
      input_rect_.top_left_x_ + input_rect_.width_ + input_spacing,
      input_rect_.top_left_y_ + (input_height - send_height) * 0.5f,
      send_width,
      send_height};

  // Messages area above input
  messages_rect_ = engine::math::RectF{
      panel_rect.top_left_x_ + padding,
      panel_rect.top_left_y_ + padding,
      panel_rect.width_ - padding * 2.0f,
      panel_rect.height_ - padding * 3.0f - input_height - input_spacing};

  // Position UI elements
  message_input_->SetPosition(
      engine::math::Vector2f{input_rect_.top_left_x_, input_rect_.top_left_y_});
  message_input_->SetSize(
      engine::math::Vector2f{input_rect_.width_, input_rect_.height_});

  send_button_->SetPosition(
      engine::math::Vector2f{send_button_rect_.top_left_x_, send_button_rect_.top_left_y_});
  send_button_->SetSize(
      engine::math::Vector2f{send_button_rect_.width_, send_button_rect_.height_});
}

void LobbyChatView::Draw(engine::render::Renderer2D& renderer) const {
  // Draw panel background
  renderer.DrawRect(panel_rect_, ui_constants::Lobby::kChatPanelColor);

  // Draw messages
  const float font_size = ui_constants::Lobby::kChatMessageFontSize;
  const float line_spacing = ui_constants::Lobby::kChatMessageSpacing;
  const float line_height = font_size + line_spacing;

  float y_pos = messages_rect_.top_left_y_ + messages_rect_.height_ - line_height;
  const float min_y = messages_rect_.top_left_y_;

  // Draw messages from newest to oldest (bottom to top)
  for (auto it = display_messages_.rbegin();
       it != display_messages_.rend() && y_pos >= min_y; ++it) {
    renderer.DrawText(*it, engine::math::Vector2f{messages_rect_.top_left_x_, y_pos},
                      font_size, ui_constants::Lobby::kChatMessageColor);
    y_pos -= line_height;
  }

  // Draw input field and send button
  message_input_->Draw(renderer);
  send_button_->Draw(renderer);
}

void LobbyChatView::ApplyStyle(ClientAssetManager& assets) {
  const auto button_texture =
      assets.GetTexture(std::string(ui_constants::kButtonTextureSmallPath));
  if (button_texture) {
    send_button_->SetTexture(button_texture);
  }
}

void LobbyChatView::AddMessage(const ChatMessage& message) {
  // Format as "Sender: content" for display
  std::string display_text;
  if (!message.sender.empty()) {
    display_text = message.sender + ": " + message.content;
  } else {
    display_text = message.content;
  }

  display_messages_.push_back(std::move(display_text));

  // Limit displayed messages
  while (display_messages_.size() > kMaxVisibleMessages) {
    display_messages_.pop_front();
  }
}

void LobbyChatView::SyncMessages(const std::deque<ChatMessage>& messages) {
  display_messages_.clear();
  for (const auto& msg : messages) {
    std::string display_text;
    if (!msg.sender.empty()) {
      display_text = msg.sender + ": " + msg.content;
    } else {
      display_text = msg.content;
    }
    display_messages_.push_back(std::move(display_text));
  }

  // Limit to max visible
  while (display_messages_.size() > kMaxVisibleMessages) {
    display_messages_.pop_front();
  }
}

void LobbyChatView::ClearMessages() {
  display_messages_.clear();
}

bool LobbyChatView::IsInputCaptured() const {
  return message_input_->IsFocused();
}

void LobbyChatView::SetInputFocused(bool focused) {
  message_input_->SetFocused(focused);
}

void LobbyChatView::UpdateInputFocus(engine::input::InputManager& input) {
  if (!input.IsMouseButtonDown(engine::input::MouseButton::kLeft)) {
    return;
  }

  const auto mouse_pos = input.GetMousePosition();
  const engine::math::Vector2f pos{static_cast<float>(mouse_pos.x),
                                   static_cast<float>(mouse_pos.y)};

  // Check if clicked within input field
  if (input_rect_.Contains(pos)) {
    message_input_->SetFocused(true);
  } else if (!send_button_rect_.Contains(pos)) {
    // Click outside input and send button - unfocus
    message_input_->SetFocused(false);
  }
}

void LobbyChatView::HandleSendAction() {
  const std::string& text = message_input_->GetText();
  if (text.empty()) {
    return;
  }

  if (on_send_ && on_send_(text)) {
    message_input_->SetText("");
  }
}

}  // namespace client

