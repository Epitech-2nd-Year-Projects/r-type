#include "profile_scene.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/input_constants.h"
#include "constants/ui_constants.h"
#include "engine/input.h"
#include "engine/render/color.h"
#include "engine/ui/layouts.h"
#include "player_profile.h"
#include "ui/menu_background.h"

namespace client {

namespace {

ui::MenuPointerConfig ProfileMenuPointerConfig() {
  return ui::MenuPointerConfig{constants::ui::kMenuPointerFramePrefix,
                               constants::ui::kMenuPointerFrameExtension,
                               constants::ui::Profile::kPointerFrameCount,
                               constants::ui::Profile::kPointerFrameDuration,
                               constants::ui::Profile::kPointerHeightFactor,
                               constants::ui::Profile::kPointerSpacing,
                               constants::ui::Profile::kPointerScaleFactor};
}

}  // namespace

ProfileScene::ProfileScene(ClientContext& context)
    : context_(context),
      selected_avatar_(context.Profile().avatar_index),
      menu_effects_(context, ProfileMenuPointerConfig(),
                    constants::ui::kMenuHoverSfxPath,
                    constants::ui::kMenuClickSfxPath) {
  auto& renderer = context_.Renderer();
  auto& assets = context_.Assets();
  assets.LoadFont(constants::ui::kTitleFont, constants::ui::kTitleFontPath);
  renderer.SetFont(std::string(constants::ui::kTitleFont));

  menu_effects_.Load();

  title_texture_ = assets.GetTexture("assets/ui/profile_text.png");
  stats_border_texture_ = assets.GetTexture("assets/ui/border.png");
  input_bg_texture_ = assets.GetTexture("assets/ui/case_profile.png");

  avatar_renderer_ = std::make_unique<ui::AvatarRenderer>(context_.Assets());
  const auto& profile = context_.Profile();

  const auto white = engine::render::Color::White();
  const auto transparent = engine::render::Color::FromBytes(0, 0, 0, 0);

  auto root =
      std::make_shared<engine::ui::StackContainer>(engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  auto padding =
      engine::ui::Insets::Uniform(constants::ui::Profile::kRootPadding);
  padding.top = constants::ui::Profile::kRootPaddingTop;
  root->SetPadding(padding);
  root->SetSpacing(constants::ui::Profile::kRootSpacing);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  auto title_slot = std::make_shared<engine::ui::BoxElement>();
  title_slot->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  title_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kButtonHeight *
      constants::ui::Profile::kButtonTextScale *
      constants::ui::Profile::kTitleScaleFactor);
  title_slot->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kCenter;
  title_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    title_rect_ = {rect.top_left_x_, rect.top_left_y_,
                   rect.width_, rect.height_};
  });
  root->AddChild(title_slot);

  auto avatar_label = std::make_shared<engine::ui::TextElement>(
      "Avatar",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Profile::kLabelFontScale),
      constants::ui::Profile::kLabelColor);
  root->AddChild(avatar_label);

  auto avatar_row = std::make_shared<engine::ui::StackContainer>(
      engine::ui::Axis::kHorizontal);
  avatar_row->SetSpacing(constants::ui::Profile::kAvatarArrowSpacing);
  avatar_row->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  avatar_row->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                                 engine::ui::VerticalAlignment::kCenter});

  avatar_left_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::Profile::kAvatarArrowWidth,
                             constants::ui::Profile::kAvatarArrowHeight},
      "<", [this]() { SelectPrevAvatar(); });
  ui_elements_.push_back(avatar_left_button_);

  auto left_arrow_slot = std::make_shared<engine::ui::BoxElement>();
  left_arrow_slot->Layout().size.width = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kAvatarArrowWidth);
  left_arrow_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kAvatarArrowHeight);
  left_arrow_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    avatar_left_button_->SetPosition({rect.top_left_x_, rect.top_left_y_});
    avatar_left_button_->SetSize({rect.width_, rect.height_});
  });

  auto avatar_slot = std::make_shared<engine::ui::BoxElement>();
  avatar_slot->Layout().size.width = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kAvatarDisplaySize);
  avatar_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kAvatarDisplaySize);
  avatar_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    avatar_position_ = {rect.top_left_x_, rect.top_left_y_};
  });

  avatar_right_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::Profile::kAvatarArrowWidth,
                             constants::ui::Profile::kAvatarArrowHeight},
      ">", [this]() { SelectNextAvatar(); });
  ui_elements_.push_back(avatar_right_button_);

  auto right_arrow_slot = std::make_shared<engine::ui::BoxElement>();
  right_arrow_slot->Layout().size.width = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kAvatarArrowWidth);
  right_arrow_slot->Layout().size.height = engine::ui::LayoutValue::Pixels(
      constants::ui::Profile::kAvatarArrowHeight);
  right_arrow_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    avatar_right_button_->SetPosition({rect.top_left_x_, rect.top_left_y_});
    avatar_right_button_->SetSize({rect.width_, rect.height_});
  });

  avatar_row->AddChild(left_arrow_slot);
  avatar_row->AddChild(avatar_slot);
  avatar_row->AddChild(right_arrow_slot);
  root->AddChild(avatar_row);

  const float input_spacing_top = 100.0f;
  
  auto input_spacer = std::make_shared<engine::ui::BoxElement>();
  input_spacer->Layout().size.height = engine::ui::LayoutValue::Pixels(input_spacing_top);
  root->AddChild(input_spacer);


  nickname_input_ = std::make_shared<engine::ui::TextInput>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::Profile::kInputWidth,
                             constants::ui::Profile::kInputHeight});
  nickname_input_->SetText(profile.nickname);
  nickname_input_->SetBackgroundColor(transparent);
  nickname_input_->SetTextColor(constants::ui::Profile::kInputTextColor);
  ui_elements_.push_back(nickname_input_);

  auto input_slot = std::make_shared<engine::ui::BoxElement>();
  input_slot->Layout().size.width =
      engine::ui::LayoutValue::Pixels(constants::ui::Profile::kInputWidth);
  input_slot->Layout().size.height =
      engine::ui::LayoutValue::Pixels(constants::ui::Profile::kInputHeight);
  input_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    nickname_input_->SetPosition({rect.top_left_x_, rect.top_left_y_});
    nickname_input_->SetSize({rect.width_, rect.height_});
  });
  root->AddChild(input_slot);

  const float stats_absolute_pos = constants::ui::Profile::kSectionSpacing + 200.0f; 
  const float stats_spacing = stats_absolute_pos - input_spacing_top;

  auto stats_spacer = std::make_shared<engine::ui::BoxElement>();
  stats_spacer->Layout().size.height =
      engine::ui::LayoutValue::Pixels(stats_spacing);
  root->AddChild(stats_spacer);

  stats_header_ = std::make_shared<engine::ui::TextElement>(
      "Statistics",
      engine::ui::FontSize::RelativeWidth(
          constants::ui::Profile::kLabelFontScale),
      white);
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

  auto button_spacer = std::make_shared<engine::ui::BoxElement>();
  button_spacer->Layout().size.height =
      engine::ui::LayoutValue::Pixels(constants::ui::Profile::kSectionSpacing);
  root->AddChild(button_spacer);

  auto button_row = std::make_shared<engine::ui::StackContainer>(
      engine::ui::Axis::kHorizontal);
  button_row->SetSpacing(constants::ui::Profile::kButtonSpacing);
  button_row->SetMainAlignment(engine::ui::StackAlignment::kCenter);

  save_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::Profile::kButtonWidth,
                             constants::ui::Profile::kButtonHeight},
      "Save", menu_effects_.WrapClick([this]() { SaveAndClose(); }));
  buttons_.push_back(save_button_);

  back_button_ = std::make_shared<engine::ui::Button>(
      engine::math::Vector2f{0.0f, 0.0f},
      engine::math::Vector2f{constants::ui::Profile::kButtonWidth,
                             constants::ui::Profile::kButtonHeight},
      "Back", menu_effects_.WrapClick([this]() { context_.OnCloseProfile(); }));
  buttons_.push_back(back_button_);

  auto save_slot = std::make_shared<engine::ui::BoxElement>();
  save_slot->Layout().size.width =
      engine::ui::LayoutValue::Pixels(constants::ui::Profile::kButtonWidth);
  save_slot->Layout().size.height =
      engine::ui::LayoutValue::Pixels(constants::ui::Profile::kButtonHeight);
  save_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    save_button_->SetPosition({rect.top_left_x_, rect.top_left_y_});
    save_button_->SetSize({rect.width_, rect.height_});
  });

  auto back_slot = std::make_shared<engine::ui::BoxElement>();
  back_slot->Layout().size.width =
      engine::ui::LayoutValue::Pixels(constants::ui::Profile::kButtonWidth);
  back_slot->Layout().size.height =
      engine::ui::LayoutValue::Pixels(constants::ui::Profile::kButtonHeight);
  back_slot->SetLayoutCallback([this](const engine::math::RectF& rect) {
    back_button_->SetPosition({rect.top_left_x_, rect.top_left_y_});
    back_button_->SetSize({rect.width_, rect.height_});
  });

  button_row->AddChild(save_slot);
  button_row->AddChild(back_slot);
  root->AddChild(button_row);

  canvas_.SetRoot(root);

  for (auto& button : buttons_) {
    button->SetColors(transparent, transparent, transparent);
    button->SetTextColor(white);
    button->SetTextScale(constants::ui::Profile::kButtonTextScale);
  }
}

void ProfileScene::Update(engine::time::TimeDelta dt) {
  auto& input = context_.Input();
  LayoutUi(context_.Renderer());

  if (input.IsMouseButtonDown(engine::input::MouseButton::kLeft)) {
    const auto mouse_pos = input.GetMousePosition();
    const auto input_pos = nickname_input_->GetPosition();
    const auto input_size = nickname_input_->GetSize();

    const bool clicked_inside = mouse_pos.x >= input_pos.x &&
                                mouse_pos.x <= input_pos.x + input_size.x &&
                                mouse_pos.y >= input_pos.y &&
                                mouse_pos.y <= input_pos.y + input_size.y;

    if (clicked_inside && !nickname_input_->IsFocused()) {
      nickname_input_->SetFocused(true);
    } else if (!clicked_inside && nickname_input_->IsFocused()) {
      nickname_input_->SetFocused(false);
    }
  }

  text_input_focused_ = nickname_input_->IsFocused();

  context_.MenuBackground().Update(dt);

  menu_effects_.Update(dt, input, buttons_);

  for (auto& elem : ui_elements_) {
    elem->Update(dt, input);
  }
  for (auto& button : buttons_) {
    button->Update(dt, input);
  }

  if (input.IsActionActive(std::string(constants::input::kActionCancel)) &&
      !text_input_focused_) {
    context_.OnCloseProfile();
  }
}

void ProfileScene::Draw(engine::render::Renderer2D& renderer) {
  context_.MenuBackground().Draw(context_.Window());
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  LayoutUi(renderer);
  DrawStatsBorder(renderer);
  DrawInputBackground(renderer);
  canvas_.LayoutAndDraw(renderer);

  DrawTitle(renderer);

  if (avatar_renderer_) {
    const float avatar_offset_y = 0.0f;
    auto pos = avatar_position_;
    pos.y += avatar_offset_y;

    avatar_renderer_->Draw(renderer, selected_avatar_, pos,
                           constants::ui::Profile::kAvatarDisplaySize);
  }

  for (auto& elem : ui_elements_) {
    elem->Draw(renderer);
  }
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  for (auto& button : buttons_) {
    button->Draw(renderer);
  }
  menu_effects_.DrawPointers(renderer, buttons_);
}

void ProfileScene::LayoutUi(engine::render::Renderer2D& renderer) {
  const auto window_size = context_.Window().GetSize();
  canvas_.SetViewportSize(
      {static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
}

void ProfileScene::DrawTitle(engine::render::Renderer2D& renderer) {
  if (!title_texture_) {
    return;
  }
  const auto tex_size = title_texture_->GetSize();
  if (tex_size.x == 0 || tex_size.y == 0) {
    return;
  }
  
  const float scale =
      std::min(title_rect_.width_ / static_cast<float>(tex_size.x),
               title_rect_.height_ / static_cast<float>(tex_size.y));
  if (scale <= 0.0f) {
    return;
  }
  
  const float draw_scale = scale * 6.5f;
  const float draw_width = static_cast<float>(tex_size.x) * draw_scale;
  const float draw_height = static_cast<float>(tex_size.y) * draw_scale;
  const float x =
      title_rect_.top_left_x_ + (title_rect_.width_ - draw_width) * 0.5f;
  const float title_offset_y = -20.0f;
  const float y =
      title_rect_.top_left_y_ + (title_rect_.height_ - draw_height) * 0.5f + title_offset_y;
  
  engine::render::SpriteDrawParams params;
  params.position = {x, y};
  params.scale = {draw_scale, draw_scale};
  renderer.DrawTexture(*title_texture_, params);
}

void ProfileScene::DrawStatsBorder(engine::render::Renderer2D& renderer) {
  if (!stats_border_texture_) {
    return;
  }
  
  if (!stats_header_ || !playtime_text_ || !deaths_text_ || 
      !highest_score_text_ || !games_played_text_) {
    return;
  }
  
  const auto tex_size = stats_border_texture_->GetSize();
  if (tex_size.x == 0 || tex_size.y == 0) {
    return;
  }
  
  const auto& header_frame = stats_header_->Frame();
  const auto& playtime_frame = playtime_text_->Frame();
  const auto& deaths_frame = deaths_text_->Frame();
  const auto& score_frame = highest_score_text_->Frame();
  const auto& games_frame = games_played_text_->Frame();
  
  const float min_x = std::min({header_frame.top_left_x_, playtime_frame.top_left_x_,
                                 deaths_frame.top_left_x_, score_frame.top_left_x_,
                                 games_frame.top_left_x_});
  const float min_y = header_frame.top_left_y_;
  const float max_x = std::max({header_frame.top_left_x_ + header_frame.width_,
                                 playtime_frame.top_left_x_ + playtime_frame.width_,
                                 deaths_frame.top_left_x_ + deaths_frame.width_,
                                 score_frame.top_left_x_ + score_frame.width_,
                                 games_frame.top_left_x_ + games_frame.width_});
  const float max_y = games_frame.top_left_y_ + games_frame.height_;
  
  const float padding_vertical = 65.0f;
  const float padding_horizontal = 165.0f;

  const float border_offset_x = 0.0f;
  const float border_offset_y = 0.0f;

  stats_rect_.top_left_x_ = min_x - padding_horizontal + border_offset_x;
  stats_rect_.top_left_y_ = min_y - padding_vertical + border_offset_y;
  stats_rect_.width_ = (max_x - min_x) + (padding_horizontal * 2.0f);
  stats_rect_.height_ = (max_y - min_y) + (padding_vertical * 2.0f);
  
  const float scale_x = stats_rect_.width_ / static_cast<float>(tex_size.x);
  const float scale_y = stats_rect_.height_ / static_cast<float>(tex_size.y);
  
  engine::render::SpriteDrawParams params;
  params.position = {stats_rect_.top_left_x_, stats_rect_.top_left_y_};
  params.scale = {scale_x, scale_y};
  
  renderer.DrawTexture(*stats_border_texture_, params);
}

void ProfileScene::SaveAndClose() {
  auto& profile = context_.Profile();
  const auto new_nickname = nickname_input_->GetText();
  if (!new_nickname.empty() &&
      new_nickname.length() <= constants::ui::Profile::kMaxNicknameLength) {
    profile.nickname = new_nickname;
  }
  profile.avatar_index = selected_avatar_;
  context_.SaveProfile();
  context_.OnCloseProfile();
}

void ProfileScene::DrawInputBackground(engine::render::Renderer2D& renderer) {
  if (!input_bg_texture_ || !nickname_input_) {
    return;
  }
  const auto input_pos = nickname_input_->GetPosition();
  const auto input_size = nickname_input_->GetSize();
  const auto tex_size = input_bg_texture_->GetSize();

  if (tex_size.x == 0 || tex_size.y == 0) {
    return;
  }

  const float scale_multiplier = 2.0f;

  const float offset_x = 0.0f;
  const float offset_y = 0.0f;

  const float base_scale = input_size.x / static_cast<float>(tex_size.x);
  const float final_scale = base_scale * scale_multiplier;

  const float draw_width = static_cast<float>(tex_size.x) * final_scale;
  const float draw_height = static_cast<float>(tex_size.y) * final_scale;

  const float x = input_pos.x + (input_size.x - draw_width) * 0.5f + offset_x;
  const float y = input_pos.y + (input_size.y - draw_height) * 0.5f + offset_y;

  engine::render::SpriteDrawParams params;
  params.position = {x, y};
  params.scale = {final_scale, final_scale};

  renderer.DrawTexture(*input_bg_texture_, params);
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

void ProfileScene::SelectPrevAvatar() {
  if (selected_avatar_ == 0) {
    selected_avatar_ =
        static_cast<std::uint8_t>(constants::ui::Profile::kAvatarCount - 1);
  } else {
    --selected_avatar_;
  }
}

void ProfileScene::SelectNextAvatar() {
  selected_avatar_ = static_cast<std::uint8_t>(
      (selected_avatar_ + 1) % constants::ui::Profile::kAvatarCount);
}

}  // namespace client
