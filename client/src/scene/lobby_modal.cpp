#include "lobby_modal.h"

#include <utility>

#include "constants/ui_constants.h"

namespace client {

LobbyModal::LobbyModal(
    std::function<bool(const std::string& room_name,
                       const std::string& max_players_text, bool is_private,
                       std::string password)>
        on_create,
    std::function<bool(const protocol::RoomSummary& room,
                       const std::string& password)>
        on_join)
    : on_create_(std::move(on_create)), on_join_(std::move(on_join)) {
  BuildModal();
}

void LobbyModal::Update(engine::time::TimeDelta dt,
                        engine::input::InputManager& input) {
  if (!show_modal_) {
    return;
  }

  auto active = ActiveElements();
  for (auto& elem : active) {
    elem->Update(dt, input);
  }
}

void LobbyModal::Layout(const engine::math::Vector2f& window_size) {
  if (!show_modal_) {
    return;
  }
  viewport_size_ = window_size;
  const float modal_x =
      (window_size.x - constants::ui::Lobby::kModalWidth) * 0.5f;
  const float modal_y =
      (window_size.y - constants::ui::Lobby::kModalHeight) * 0.5f;
  modal_rect_ = {modal_x, modal_y, constants::ui::Lobby::kModalWidth,
                 constants::ui::Lobby::kModalHeight};

  if (modal_mode_ == ModalMode::kCreate) {
    room_name_input_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalRoomNameInputY});
    room_name_input_->SetSize(
        {constants::ui::Lobby::kModalWidth -
             constants::ui::Lobby::kModalPaddingX * 2.0f,
         constants::ui::Lobby::kFieldHeight});

    max_players_input_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalMaxPlayersInputY});
    max_players_input_->SetSize(
        {constants::ui::Lobby::kModalMaxPlayersWidth,
         constants::ui::Lobby::kFieldHeight});

    privacy_button_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPrivacyButtonX,
         max_players_input_->GetPosition().y -
             constants::ui::Lobby::kModalPrivacyButtonOffsetY});
    privacy_button_->SetSize(
        {constants::ui::Lobby::kModalActionButtonWidth,
         constants::ui::Lobby::kButtonHeight});

    if (modal_private_) {
      password_input_->SetPosition(
          {modal_x + constants::ui::Lobby::kModalPaddingX,
           modal_y + constants::ui::Lobby::kModalPasswordInputY});
      password_input_->SetSize(
          {constants::ui::Lobby::kModalWidth -
               constants::ui::Lobby::kModalPaddingX * 2.0f,
           constants::ui::Lobby::kFieldHeight});
    }

    primary_button_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalPrimaryButtonY});
    cancel_button_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalWidth -
             cancel_button_->GetSize().x -
             constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalPrimaryButtonY});
  } else if (modal_mode_ == ModalMode::kJoinPrivate) {
    password_input_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalPasswordJoinInputY});
    password_input_->SetSize(
        {constants::ui::Lobby::kModalWidth -
             constants::ui::Lobby::kModalPaddingX * 2.0f,
         constants::ui::Lobby::kFieldHeight});

    primary_button_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalJoinButtonY});
    cancel_button_->SetPosition(
        {modal_x + constants::ui::Lobby::kModalWidth -
             cancel_button_->GetSize().x -
             constants::ui::Lobby::kModalPaddingX,
         modal_y + constants::ui::Lobby::kModalJoinButtonY});
  }

  primary_button_->SetSize({constants::ui::Lobby::kModalActionButtonWidth,
                            constants::ui::Lobby::kButtonHeight});
  cancel_button_->SetSize({constants::ui::Lobby::kModalActionButtonWidth,
                           constants::ui::Lobby::kButtonHeight});
}

void LobbyModal::Draw(engine::render::Renderer2D& renderer) const {
  if (!show_modal_) {
    return;
  }

  const float width = modal_rect_.width_;
  const float height = modal_rect_.height_;
  const float x = modal_rect_.top_left_x_;
  const float y = modal_rect_.top_left_y_;

  renderer.DrawRect(
      {0.0f, 0.0f, viewport_size_.x, viewport_size_.y},
      constants::ui::Lobby::kOverlayColor);
  renderer.DrawRect({x, y, width, height}, constants::ui::Lobby::kPanelColor);

  const std::string title =
      modal_mode_ == ModalMode::kCreate ? "Create a room" : "Enter password";
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  renderer.DrawText(
      title,
      {x + constants::ui::Lobby::kModalPaddingX,
       y + constants::ui::Lobby::kModalTitleOffsetY},
      constants::ui::Lobby::kModalTitleFontSize,
      constants::ui::Lobby::kAccentColor);

  renderer.SetFont(std::string(constants::ui::kBodyFont));
  if (modal_mode_ == ModalMode::kCreate) {
    renderer.DrawText(
        "Room name",
        {x + constants::ui::Lobby::kModalPaddingX,
         y + constants::ui::Lobby::kModalLabelRow1Y},
        constants::ui::Lobby::kModalLabelFontSize,
        engine::render::Color::White());
    renderer.DrawText(
        "Max players",
        {x + constants::ui::Lobby::kModalPaddingX,
         y + constants::ui::Lobby::kModalLabelRow2Y},
        constants::ui::Lobby::kModalLabelFontSize,
        engine::render::Color::White());
    renderer.DrawText(
        modal_private_ ? "Private lobby" : "Public lobby",
        {x + constants::ui::Lobby::kModalLabelValueX,
         y + constants::ui::Lobby::kModalLabelRow2Y},
        constants::ui::Lobby::kModalLabelFontSize,
        constants::ui::Lobby::kSoftTextColor);
    if (modal_private_) {
      renderer.DrawText(
          "Password (4 digits)",
          {x + constants::ui::Lobby::kModalPaddingX,
           y + constants::ui::Lobby::kModalPrivateLabelY},
          constants::ui::Lobby::kModalLabelFontSize,
          constants::ui::Lobby::kSoftTextColor);
    }
  } else {
    std::string subtitle = "Join " + pending_join_room_name_;
    renderer.DrawText(
        subtitle,
        {x + constants::ui::Lobby::kModalPaddingX,
         y + constants::ui::Lobby::kModalLabelRow1Y},
        constants::ui::Lobby::kModalSubtitleFontSize,
        constants::ui::Lobby::kSoftTextColor);
  }

  auto active = ActiveElements();
  for (auto& elem : active) {
    elem->Draw(renderer);
  }
}

void LobbyModal::HandleFocus(const engine::input::InputManager& input) {
  if (!show_modal_) {
    return;
  }
  const auto pos = input.GetMousePosition();
  auto set_focus = [&](const std::shared_ptr<engine::ui::TextInput>& field) {
    if (!field) {
      return;
    }
    engine::math::RectF rect{field->GetPosition(), field->GetSize()};
    field->SetFocused(rect.Contains(pos));
  };

  if (modal_mode_ == ModalMode::kCreate) {
    set_focus(room_name_input_);
    set_focus(max_players_input_);
    if (modal_private_) {
      set_focus(password_input_);
    }
  } else {
    set_focus(password_input_);
  }
}

void LobbyModal::OpenCreate() {
  modal_mode_ = ModalMode::kCreate;
  modal_private_ = false;
  primary_button_->SetText("Create");
  privacy_button_->SetText("Public");
  room_name_input_->SetText("");
  max_players_input_->SetText("4");
  password_input_->SetPlaceholder("Private password (optional)");
  password_input_->SetText("");
  show_modal_ = true;
}

void LobbyModal::OpenJoin(const protocol::RoomSummary& room) {
  modal_mode_ = ModalMode::kJoinPrivate;
  pending_join_room_code_ = room.room_code;
  pending_join_room_name_ = room.room_name;
  primary_button_->SetText("Join");
  password_input_->SetPlaceholder("Enter 4-digit password");
  password_input_->SetText("");
  show_modal_ = true;
}

void LobbyModal::Close() {
  show_modal_ = false;
  pending_join_room_code_.clear();
  pending_join_room_name_.clear();
}

bool LobbyModal::IsInputCaptured() const {
  if (!show_modal_) {
    return false;
  }
  if (room_name_input_ && room_name_input_->IsFocused()) {
    return true;
  }
  if (max_players_input_ && max_players_input_->IsFocused()) {
    return true;
  }
  if (password_input_ && password_input_->IsFocused()) {
    return true;
  }
  return false;
}

void LobbyModal::BuildModal() {
  room_name_input_ =
      std::make_shared<engine::ui::TextInput>(engine::math::Vector2f{},
                                              engine::math::Vector2f{});
  room_name_input_->SetPlaceholder("Room name");

  max_players_input_ =
      std::make_shared<engine::ui::TextInput>(engine::math::Vector2f{},
                                              engine::math::Vector2f{});
  max_players_input_->SetPlaceholder("Max players (1-255)");
  max_players_input_->SetText("4");

  privacy_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kModalActionButtonWidth,
                             constants::ui::Lobby::kButtonHeight},
      "Public", [this]() {
        modal_private_ = !modal_private_;
        privacy_button_->SetText(modal_private_ ? "Private" : "Public");
        if (!modal_private_) {
          password_input_->SetText("");
        }
      });

  password_input_ =
      std::make_shared<engine::ui::TextInput>(engine::math::Vector2f{},
                                              engine::math::Vector2f{});
  password_input_->SetPlaceholder("4-digit password");

  primary_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kModalActionButtonWidth,
                             constants::ui::Lobby::kButtonHeight},
      "Create", [this]() { ApplyPrimaryAction(); });

  cancel_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{},
      engine::math::Vector2f{constants::ui::Lobby::kModalActionButtonWidth,
                             constants::ui::Lobby::kButtonHeight},
      "Cancel", [this]() { Close(); });

  create_elements_ = {room_name_input_,   max_players_input_, privacy_button_,
                      primary_button_,    cancel_button_};
  join_elements_ = {password_input_, primary_button_, cancel_button_};
}

void LobbyModal::ApplyPrimaryAction() {
  if (modal_mode_ == ModalMode::kCreate) {
    if (!on_create_) {
      return;
    }
    std::string name = room_name_input_->GetText();
    std::string max_players_text = max_players_input_->GetText();
    std::string password = password_input_->GetText();
    const bool accepted =
        on_create_(name, max_players_text, modal_private_, std::move(password));
    if (accepted) {
      Close();
    }
  } else {
    if (!on_join_) {
      return;
    }
    const std::string password = password_input_->GetText();
    protocol::RoomSummary target{};
    target.room_code = pending_join_room_code_;
    target.room_name = pending_join_room_name_;
    target.is_private = true;
    const bool accepted = on_join_(target, password);
    if (accepted) {
      Close();
    }
  }
}

std::vector<std::shared_ptr<engine::ui::Widget>> LobbyModal::ActiveElements()
    const {
  std::vector<std::shared_ptr<engine::ui::Widget>> active =
      modal_mode_ == ModalMode::kCreate ? create_elements_ : join_elements_;
  if (modal_mode_ == ModalMode::kCreate && modal_private_) {
    active.push_back(password_input_);
  }
  return active;
}

}  // namespace client
