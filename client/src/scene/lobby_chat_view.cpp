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

constexpr engine::render::Color kTitleBarColor =
    engine::render::Color::FromBytes(20, 30, 50, 240);
constexpr engine::render::Color kMinimizeButtonColor =
    engine::render::Color::FromBytes(80, 100, 140, 255);
constexpr engine::render::Color kResizeHandleColor =
    engine::render::Color::FromBytes(60, 80, 120, 200);
constexpr float kTitleFontSize = 14.0f;

engine::render::Color GenerateNameColor(std::string_view name) {
  if (name.empty()) {
    return engine::render::Color::White();
  }
  std::size_t hash = 0;
  for (char c : name) {
    hash = c + (hash << 6) + (hash << 16) - hash;
  }
  
  // Use HSL approach for pleasing colors (high saturation/lightness)
  // Simple RGB generation for now to avoid advanced math dep
  const std::uint8_t r = 100 + (hash % 156);
  const std::uint8_t g = 100 + ((hash >> 8) % 156);
  const std::uint8_t b = 100 + ((hash >> 16) % 156);
  
  return engine::render::Color::FromBytes(r, g, b, 255);
}

}  // namespace

LobbyChatView::LobbyChatView(ClientContext& context, SendCallback on_send)
    : context_(context),
      on_send_(std::move(on_send)),
      message_input_(std::make_shared<engine::ui::TextInput>(
          engine::math::Vector2f{0.0f, 0.0f},
          engine::math::Vector2f{200.0f,
                                 ui_constants::Lobby::kChatInputHeight})),
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

  const auto mouse_pos = input.GetMousePosition();
  const engine::math::Vector2f mouse{static_cast<float>(mouse_pos.x),
                                     static_cast<float>(mouse_pos.y)};
  const bool mouse_down =
      input.IsMouseButtonDown(engine::input::MouseButton::kLeft);

  // Handle minimize button click
  if (mouse_down && !mouse_was_down_ && minimize_button_rect_.Contains(mouse)) {
    ToggleMinimized();
    mouse_was_down_ = mouse_down;
    return;
  }

  // Handle dragging and resizing
  UpdateDragging(input);
  if (!minimized_) {
    UpdateResizing(input);
  }

  mouse_was_down_ = mouse_down;

  if (minimized_) {
    return;
  }

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

void LobbyChatView::SetDefaultBounds(const engine::math::RectF& panel_rect) {
  if (panel_rect_.width_ == 0.0f && panel_rect_.height_ == 0.0f) {
    panel_rect_ = panel_rect;
  }
}

void LobbyChatView::Layout() {
  const float padding = ui_constants::Lobby::kChatPanelPadding;
  const float input_height = ui_constants::Lobby::kChatInputHeight;
  const float input_spacing = ui_constants::Lobby::kChatInputSpacing;
  const float send_width = ui_constants::Lobby::kChatSendButtonWidth;
  const float send_height = ui_constants::Lobby::kChatSendButtonHeight;

  // Title bar at top
  title_bar_rect_ = engine::math::RectF{panel_rect_.top_left_x_,
                                        panel_rect_.top_left_y_,
                                        panel_rect_.width_, kTitleBarHeight};

  // Minimize button in title bar (right side)
  const float min_btn_size = kTitleBarHeight - 8.0f;
  minimize_button_rect_ = engine::math::RectF{
      panel_rect_.top_left_x_ + panel_rect_.width_ - min_btn_size - 4.0f,
      panel_rect_.top_left_y_ + 4.0f, min_btn_size, min_btn_size};

  // Resize handle at bottom-right corner
  resize_handle_rect_ = engine::math::RectF{
      panel_rect_.top_left_x_ + panel_rect_.width_ - kResizeHandleSize,
      panel_rect_.top_left_y_ + panel_rect_.height_ - kResizeHandleSize,
      kResizeHandleSize, kResizeHandleSize};

  if (minimized_) {
    return;
  }

  const float content_top = panel_rect_.top_left_y_ + kTitleBarHeight + padding;

  // Input area at bottom
  input_rect_ = engine::math::RectF{
      panel_rect_.top_left_x_ + padding,
      panel_rect_.top_left_y_ + panel_rect_.height_ - padding - input_height,
      panel_rect_.width_ - padding * 2.0f - send_width - input_spacing,
      input_height};

  // Send button to the right of input
  send_button_rect_ = engine::math::RectF{
      input_rect_.top_left_x_ + input_rect_.width_ + input_spacing,
      input_rect_.top_left_y_ + (input_height - send_height) * 0.5f, send_width,
      send_height};

  // Messages area between title bar and input
  messages_rect_ = engine::math::RectF{
      panel_rect_.top_left_x_ + padding, content_top,
      panel_rect_.width_ - padding * 2.0f,
      input_rect_.top_left_y_ - content_top - input_spacing};

  // Position UI elements
  message_input_->SetPosition(
      engine::math::Vector2f{input_rect_.top_left_x_, input_rect_.top_left_y_});
  message_input_->SetSize(
      engine::math::Vector2f{input_rect_.width_, input_rect_.height_});

  send_button_->SetPosition(engine::math::Vector2f{send_button_rect_.top_left_x_,
                                                   send_button_rect_.top_left_y_});
  send_button_->SetSize(
      engine::math::Vector2f{send_button_rect_.width_, send_button_rect_.height_});
}

void LobbyChatView::Layout(const engine::math::RectF& panel_rect) {
  SetDefaultBounds(panel_rect);
  Layout();
}

void LobbyChatView::Draw(engine::render::Renderer2D& renderer) const {
  // Draw panel background
  if (minimized_) {
    renderer.DrawRect(title_bar_rect_, kTitleBarColor);
  } else {
    renderer.DrawRect(panel_rect_, ui_constants::Lobby::kChatPanelColor);
    renderer.DrawRect(title_bar_rect_, kTitleBarColor);
  }

  // Draw title text
  renderer.DrawText("Chat", engine::math::Vector2f{title_bar_rect_.top_left_x_ + 8.0f,
                                                   title_bar_rect_.top_left_y_ + 6.0f},
                    kTitleFontSize, engine::render::Color::White());

  // Draw minimize button
  renderer.DrawRect(minimize_button_rect_, kMinimizeButtonColor);
  const char* min_text = minimized_ ? "+" : "-";
  renderer.DrawText(min_text,
                    engine::math::Vector2f{minimize_button_rect_.top_left_x_ + 5.0f,
                                           minimize_button_rect_.top_left_y_ + 2.0f},
                    kTitleFontSize, engine::render::Color::White());

  if (minimized_) {
    return;
  }

  // Draw resize handle
  renderer.DrawRect(resize_handle_rect_, kResizeHandleColor);

  // Draw messages
  const float font_size = ui_constants::Lobby::kChatMessageFontSize;
  const float line_spacing = ui_constants::Lobby::kChatMessageSpacing;
  const float line_height = font_size + line_spacing;

  float y_pos = messages_rect_.top_left_y_ + messages_rect_.height_ - line_height;
  const float min_y = messages_rect_.top_left_y_;

  for (auto it = display_messages_.rbegin();
       it != display_messages_.rend() && y_pos >= min_y; ++it) {
    const auto& msg = *it;
    
    // Draw sender name
    if (!msg.sender.empty()) {
      const std::string sender_text = msg.sender + ": ";
      const auto name_color = GenerateNameColor(msg.sender);
      renderer.DrawText(sender_text,
                        engine::math::Vector2f{messages_rect_.top_left_x_, y_pos},
                        font_size, name_color);
                        
      // Measure name width to offset message content
      const float name_width = renderer.MeasureText(sender_text, font_size).x;
      
      renderer.DrawText(msg.content, 
                        engine::math::Vector2f{messages_rect_.top_left_x_ + name_width, y_pos},
                        font_size, ui_constants::Lobby::kChatMessageColor);
    } else {
      // System message or self
      renderer.DrawText(msg.content,
                        engine::math::Vector2f{messages_rect_.top_left_x_, y_pos},
                        font_size, ui_constants::Lobby::kChatMessageColor);
    }
    
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
  display_messages_.push_back(message);

  while (display_messages_.size() > kMaxVisibleMessages) {
    display_messages_.pop_front();
  }
}

void LobbyChatView::SyncMessages(const std::deque<ChatMessage>& messages) {
  display_messages_ = messages;

  while (display_messages_.size() > kMaxVisibleMessages) {
    display_messages_.pop_front();
  }
}

void LobbyChatView::ClearMessages() { display_messages_.clear(); }

bool LobbyChatView::IsInputCaptured() const {
  return !minimized_ && message_input_->IsFocused();
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

  if (input_rect_.Contains(pos)) {
    message_input_->SetFocused(true);
  } else if (!send_button_rect_.Contains(pos)) {
    message_input_->SetFocused(false);
  }
}

void LobbyChatView::UpdateDragging(engine::input::InputManager& input) {
  const auto mouse_pos = input.GetMousePosition();
  const engine::math::Vector2f mouse{static_cast<float>(mouse_pos.x),
                                     static_cast<float>(mouse_pos.y)};
  const bool mouse_down =
      input.IsMouseButtonDown(engine::input::MouseButton::kLeft);

  if (mouse_down && !mouse_was_down_) {
    // Start dragging if clicking on title bar (but not minimize button)
    if (title_bar_rect_.Contains(mouse) &&
        !minimize_button_rect_.Contains(mouse)) {
      dragging_ = true;
      drag_offset_ = engine::math::Vector2f{
          mouse.x - panel_rect_.top_left_x_, mouse.y - panel_rect_.top_left_y_};
    }
  } else if (!mouse_down) {
    dragging_ = false;
  }

  if (dragging_) {
    panel_rect_.top_left_x_ = mouse.x - drag_offset_.x;
    panel_rect_.top_left_y_ = mouse.y - drag_offset_.y;
    ClampToScreen();
    Layout();
  }
}

void LobbyChatView::UpdateResizing(engine::input::InputManager& input) {
  const auto mouse_pos = input.GetMousePosition();
  const engine::math::Vector2f mouse{static_cast<float>(mouse_pos.x),
                                     static_cast<float>(mouse_pos.y)};
  const bool mouse_down =
      input.IsMouseButtonDown(engine::input::MouseButton::kLeft);

  if (mouse_down && !mouse_was_down_) {
    if (resize_handle_rect_.Contains(mouse)) {
      resizing_ = true;
      resize_start_size_ =
          engine::math::Vector2f{panel_rect_.width_, panel_rect_.height_};
      resize_start_mouse_ = mouse;
    }
  } else if (!mouse_down) {
    resizing_ = false;
  }

  if (resizing_) {
    const float dx = mouse.x - resize_start_mouse_.x;
    const float dy = mouse.y - resize_start_mouse_.y;
    panel_rect_.width_ = std::max(kMinWidth, resize_start_size_.x + dx);
    panel_rect_.height_ = std::max(kMinHeight, resize_start_size_.y + dy);
    ClampToScreen();
    Layout();
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

void LobbyChatView::ClampToScreen() {
  const auto window_size = context_.Window().GetSize();
  const float max_x =
      static_cast<float>(window_size.x) - panel_rect_.width_;
  const float max_y =
      static_cast<float>(window_size.y) - panel_rect_.height_;

  panel_rect_.top_left_x_ = std::max(0.0f, std::min(panel_rect_.top_left_x_, max_x));
  panel_rect_.top_left_y_ = std::max(0.0f, std::min(panel_rect_.top_left_y_, max_y));
}

}  // namespace client


