#include "profile_scene.h"

#include <iomanip>
#include <sstream>
#include <string>

#include "client_context.h"
#include "constants/input_constants.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"
#include "player_profile.h"
#include "ui/menu_background.h"

namespace client {

ProfileScene::ProfileScene(ClientContext& context) : context_(context) {
  const auto& profile = context_.Profile();

  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetPadding(engine::ui::Insets::Uniform(
      constants::ui::Profile::kRootPadding));
  root->SetSpacing(constants::ui::Profile::kRootSpacing);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  // Title
  title_ = std::make_shared<engine::ui::TextElement>(
      "PLAYER PROFILE",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Profile::kTitleFontScale),
      constants::ui::Profile::kTitleColor);
  root->AddChild(title_);

  // Nickname section
  nickname_label_ = std::make_shared<engine::ui::TextElement>(
      "Nickname",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Profile::kLabelFontScale),
      constants::ui::Profile::kLabelColor);
  root->AddChild(nickname_label_);

  nickname_input_ = std::make_shared<engine::ui::TextInput>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::Profile::kInputWidth,
                             constants::ui::Profile::kInputHeight});
  nickname_input_->SetText(profile.nickname);
  nickname_input_->SetBackgroundColor(constants::ui::Profile::kInputBgColor);
  nickname_input_->SetTextColor(constants::ui::Profile::kInputTextColor);
  ui_elements_.push_back(nickname_input_);

  auto input_slot = std::make_shared<engine::ui::BoxElement>();
  input_slot->Layout().size.width = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kInputWidth);
  input_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kInputHeight);
  input_slot->SetLayoutCallback(
      [this](const engine::math::RectF& rect) {
        nickname_input_->SetPosition({rect.top_left_x_, rect.top_left_y_});
        nickname_input_->SetSize({rect.width_, rect.height_});
      });
  root->AddChild(input_slot);

  // Stats section
  auto stats_spacer = std::make_shared<engine::ui::BoxElement>();
  stats_spacer->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kSectionSpacing);
  root->AddChild(stats_spacer);

  stats_header_ = std::make_shared<engine::ui::TextElement>(
      "Statistics",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Profile::kLabelFontScale),
      constants::ui::Profile::kLabelColor);
  root->AddChild(stats_header_);

  std::string playtime_str;
  FormatPlaytime(profile.stats.total_playtime_seconds, playtime_str);
  playtime_text_ = std::make_shared<engine::ui::TextElement>(
      "Total Playtime: " + playtime_str,
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Profile::kValueFontScale),
      constants::ui::Profile::kValueColor);
  root->AddChild(playtime_text_);

  deaths_text_ = std::make_shared<engine::ui::TextElement>(
      "Total Deaths: " + std::to_string(profile.stats.total_deaths),
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Profile::kValueFontScale),
      constants::ui::Profile::kValueColor);
  root->AddChild(deaths_text_);

  highest_score_text_ = std::make_shared<engine::ui::TextElement>(
      "Highest Score: " + std::to_string(profile.stats.highest_score),
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Profile::kValueFontScale),
      constants::ui::Profile::kValueColor);
  root->AddChild(highest_score_text_);

  games_played_text_ = std::make_shared<engine::ui::TextElement>(
      "Games Played: " + std::to_string(profile.stats.games_played),
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Profile::kValueFontScale),
      constants::ui::Profile::kValueColor);
  root->AddChild(games_played_text_);

  // Button row
  auto button_spacer = std::make_shared<engine::ui::BoxElement>();
  button_spacer->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kSectionSpacing);
  root->AddChild(button_spacer);

  auto button_row = std::make_shared<engine::ui::StackContainer>(
      engine::ui::Axis::kHorizontal);
  button_row->SetSpacing(constants::ui::Profile::kButtonSpacing);
  button_row->SetMainAlignment(engine::ui::StackAlignment::kCenter);

  save_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::Profile::kButtonWidth,
                             constants::ui::Profile::kButtonHeight},
      "Save", [this]() { SaveAndClose(); });
  ui_elements_.push_back(save_button_);

  back_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::Profile::kButtonWidth,
                             constants::ui::Profile::kButtonHeight},
      "Back", [this]() { context_.OnCloseProfile(); });
  ui_elements_.push_back(back_button_);

  auto save_slot = std::make_shared<engine::ui::BoxElement>();
  save_slot->Layout().size.width = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kButtonWidth);
  save_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kButtonHeight);
  save_slot->SetLayoutCallback(
      [this](const engine::math::RectF& rect) {
        save_button_->SetPosition({rect.top_left_x_, rect.top_left_y_});
        save_button_->SetSize({rect.width_, rect.height_});
      });

  auto back_slot = std::make_shared<engine::ui::BoxElement>();
  back_slot->Layout().size.width = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kButtonWidth);
  back_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kButtonHeight);
  back_slot->SetLayoutCallback(
      [this](const engine::math::RectF& rect) {
        back_button_->SetPosition({rect.top_left_x_, rect.top_left_y_});
        back_button_->SetSize({rect.width_, rect.height_});
      });

  button_row->AddChild(save_slot);
  button_row->AddChild(back_slot);
  root->AddChild(button_row);

  canvas_.SetRoot(root);
}

void ProfileScene::Update(engine::time::TimeDelta dt) {
  auto& input = context_.Input();
  LayoutUi(context_.Renderer());

  if (input.IsMouseButtonDown(engine::input::MouseButton::kLeft)) {
    const auto mouse_pos = input.GetMousePosition();
    const auto input_pos = nickname_input_->GetPosition();
    const auto input_size = nickname_input_->GetSize();

    const bool clicked_inside =
        mouse_pos.x >= input_pos.x && mouse_pos.x <= input_pos.x + input_size.x &&
        mouse_pos.y >= input_pos.y && mouse_pos.y <= input_pos.y + input_size.y;

    if (clicked_inside && !nickname_input_->IsFocused()) {
      nickname_input_->SetFocused(true);
    } else if (!clicked_inside && nickname_input_->IsFocused()) {
      nickname_input_->SetFocused(false);
    }
  }

  text_input_focused_ = nickname_input_->IsFocused();

  for (auto& elem : ui_elements_) {
    elem->Update(dt, input);
  }

  if (input.IsActionActive(std::string(constants::input::kActionCancel)) &&
      !text_input_focused_) {
    context_.OnCloseProfile();
  }
}

void ProfileScene::Draw(engine::render::Renderer2D& renderer) {
  context_.MenuBackground().Draw(context_.Window());
  LayoutUi(renderer);
  canvas_.LayoutAndDraw(renderer);
  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
}

void ProfileScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto window_size = context_.Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
}

void ProfileScene::SaveAndClose() {
  auto& profile = context_.Profile();
  const auto new_nickname = nickname_input_->GetText();
  if (!new_nickname.empty() &&
      new_nickname.length() <= constants::ui::Profile::kMaxNicknameLength) {
    profile.nickname = new_nickname;
  }
  context_.SaveProfile();
  context_.OnCloseProfile();
}

void ProfileScene::FormatPlaytime(std::uint64_t seconds,
                                   std::string& out) const {
  const auto hours = seconds / 3600;
  const auto minutes = (seconds % 3600) / 60;
  const auto secs = seconds % 60;

  std::ostringstream ss;
  if (hours > 0) {
    ss << hours << "h ";
  }
  if (minutes > 0 || hours > 0) {
    ss << minutes << "m ";
  }
  ss << secs << "s";
  out = ss.str();
}

}  // namespace client
