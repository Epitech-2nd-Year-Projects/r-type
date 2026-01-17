#include <gtest/gtest.h>

#include <string>

#include "constants/ui_constants.h"
#include "engine/input.h"
#include "input/key_binding_service.h"
#include "scene/lobby_modal.h"

TEST(LobbyModalTest, CapturesInputWhenTextFieldFocused) {
  client::LobbyModal modal(
      [](const std::string&, const std::string&, bool, std::string,
         protocol::Difficulty) { return false; },
      [](const protocol::RoomSummary&, const std::string&) { return false; });

  modal.OpenCreate();
  const engine::math::Vector2f window_size{800.0f, 600.0f};
  modal.Layout(window_size);

  const float modal_x =
      (window_size.x - client::constants::ui::Lobby::kModalWidth) * 0.5f;
  const float modal_y =
      (window_size.y - client::constants::ui::Lobby::kModalHeight) * 0.5f;
  const float field_x = modal_x + client::constants::ui::Lobby::kModalPaddingX;
  const float field_y =
      modal_y + client::constants::ui::Lobby::kModalRoomNameInputY;

  engine::input::InputManager input;
  input.SetMousePosition({field_x + 1.0f, field_y + 1.0f});
  modal.HandleFocus(input);

  EXPECT_TRUE(modal.IsInputCaptured());
}

TEST(LobbyModalTest, DoesNotCaptureInputWhenClosed) {
  client::LobbyModal modal(
      [](const std::string&, const std::string&, bool, std::string,
         protocol::Difficulty) { return false; },
      [](const protocol::RoomSummary&, const std::string&) { return false; });

  EXPECT_FALSE(modal.IsOpen());
  EXPECT_FALSE(modal.IsInputCaptured());
}

TEST(KeyBindingServiceTest, ReportsConflictingBinding) {
  client::KeyBindingService service;

  const auto result = service.UpdateBinding(client::GameAction::kMoveDown,
                                            engine::input::Key::kZ);

  EXPECT_EQ(result.status, client::KeyBindingUpdateStatus::kConflict);
  EXPECT_EQ(result.message, "Key already bound to Move Up");
}
