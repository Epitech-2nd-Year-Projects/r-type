#include "lobby_room_list_view.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#include "client_asset_manager.h"
#include "client_context.h"
#include "constants/ui_constants.h"
#include "ui/menu_background.h"

namespace client {

namespace {

constexpr std::array<std::string_view, 33> kAreaTextures = {
    "assets/ui/area/Area_Abyss.png",
    "assets/ui/area/Area_Art_City_of_Tears.png",
    "assets/ui/area/Area_Art_Godshome.png",
    "assets/ui/area/Area_Art_Junk_Pit.png",
    "assets/ui/area/Area_Art_Mantis_Village.png",
    "assets/ui/area/Area_Black_Egg.png",
    "assets/ui/area/Area_Cliffs.png",
    "assets/ui/area/Area_Colosseum.png",
    "assets/ui/area/Area_Crystal_Mines.png",
    "assets/ui/area/Area_Deepnest.png",
    "assets/ui/area/Area_Dirtmouth.png",
    "assets/ui/area/Area_Dream_Well.png",
    "assets/ui/area/Area_Forgotten Crossroads.png",
    "assets/ui/area/Area_Fungal_Wastes.png",
    "assets/ui/area/Area_Garz_Den.png",
    "assets/ui/area/Area_Green_Path.png",
    "assets/ui/area/Area_Hive.png",
    "assets/ui/area/Area_Kingdoms_Edge.png",
    "assets/ui/area/Area_Kings_Pass.png",
    "assets/ui/area/Area_Kings_Station.png",
    "assets/ui/area/Area_Lurian_Tower.png",
    "assets/ui/area/Area_Mage_Tower.png",
    "assets/ui/area/Area_Palace_Grounds.png",
    "assets/ui/area/Area_Queen_Station.png",
    "assets/ui/area/Area_Resting_Grounds.png",
    "assets/ui/area/Area_Royal_Gardens.png",
    "assets/ui/area/Area_Royal_Quarter.png",
    "assets/ui/area/Area_Shaman_Temple.png",
    "assets/ui/area/Area_Teacher_archive.png",
    "assets/ui/area/Area_Tram.png",
    "assets/ui/area/Area_Tram_Lower.png",
    "assets/ui/area/Area_Waterways.png",
    "assets/ui/area/Area_White_Palace.png",
};

ui::MenuPointerConfig LobbyPointerConfig() {
  return ui::MenuPointerConfig{
      constants::ui::kMenuPointerFramePrefix,
      constants::ui::kMenuPointerFrameExtension,
      constants::ui::OptionsMenu::kPointerFrameCount,
      constants::ui::OptionsMenu::kPointerFrameDuration,
      constants::ui::OptionsMenu::kPointerHeightFactor,
      constants::ui::Lobby::kPointerSpacing,
      constants::ui::OptionsMenu::kPointerScaleFactor};
}

std::size_t HashRooms(const std::vector<protocol::RoomSummary>& rooms) {
  std::size_t value = rooms.size();
  for (const auto& room : rooms) {
    std::size_t local = std::hash<std::string>{}(room.room_code);
    local ^= std::hash<std::string>{}(room.room_name) +
             constants::ui::Lobby::kGoldenHashRatio + (local << 6) +
             (local >> 2);
    local ^=
        static_cast<std::size_t>(room.player_count + 31u * room.max_players);
    local ^= static_cast<std::size_t>(
        room.is_private ? constants::ui::Lobby::kHashPrivateSalt
                        : constants::ui::Lobby::kHashPublicSeed);
    local ^= static_cast<std::size_t>(room.started ? 0xfeedbabe : 0x0);
    value ^= local + constants::ui::Lobby::kGoldenHashRatio + (value << 6) +
             (value >> 2);
  }
  return value;
}

void LoadAreaTextures(
    ClientAssetManager& assets,
    std::vector<std::shared_ptr<engine::render::Texture2D>>& textures) {
  if (!textures.empty()) {
    return;
  }
  textures.reserve(kAreaTextures.size());
  for (const auto path : kAreaTextures) {
    if (auto tex = assets.GetTexture(path)) {
      textures.push_back(tex);
    }
  }
}

void LoadFrameSequence(
    ClientAssetManager& assets, std::string_view prefix,
    std::string_view extension, int count,
    std::vector<std::shared_ptr<engine::render::Texture2D>>& frames) {
  if (!frames.empty()) {
    return;
  }
  frames.reserve(static_cast<std::size_t>(count));
  for (int i = 0; i < count; ++i) {
    std::ostringstream path;
    path << prefix << std::setw(4) << std::setfill('0') << i << extension;
    if (auto tex = assets.GetTexture(path.str())) {
      frames.push_back(tex);
    }
  }
}

engine::math::Vector2f TextureSizeOrFallback(
    const std::vector<std::shared_ptr<engine::render::Texture2D>>& frames,
    float fallback_width, float fallback_height) {
  if (!frames.empty() && frames.front()) {
    const auto size = frames.front()->GetSize();
    if (size.x > 0 && size.y > 0) {
      return {static_cast<float>(size.x), static_cast<float>(size.y)};
    }
  }
  return {fallback_width, fallback_height};
}

std::size_t FrameIndex(float elapsed, float frame_duration,
                       std::size_t frame_count) {
  if (frame_count == 0 || frame_duration <= 0.0f) {
    return 0;
  }
  const auto index = static_cast<std::size_t>(elapsed / frame_duration);
  return std::min(frame_count - 1, index);
}

std::string BuildRoomRightText(const protocol::RoomSummary& room) {
  return std::to_string(room.player_count) + "/" +
         std::to_string(room.max_players);
}

std::string ExtractRoomId(std::string_view code) {
  std::string digits;
  for (const char c : code) {
    if (c >= '0' && c <= '9') {
      digits.push_back(c);
    }
  }
  if (digits.empty()) {
    return std::string(code);
  }
  return digits;
}

std::string BuildRoomCenterText(const protocol::RoomSummary& room) {
  const std::string visibility = room.is_private ? "private" : "public";
  return room.room_name + " (" + visibility + ")";
}

void ApplyFrameTexture(
    const std::vector<std::shared_ptr<engine::ui::Button>>& buttons,
    const std::shared_ptr<engine::render::Texture2D>& texture) {
  if (!texture) {
    return;
  }
  for (auto& button : buttons) {
    button->SetTexture(texture);
  }
}

float RoomListContentHeight(std::size_t count, float item_height,
                            float spacing) {
  if (count == 0) {
    return 0.0f;
  }
  return static_cast<float>(count) * item_height +
         static_cast<float>(count - 1) * spacing;
}

float SnapScrollOffset(float value, float step, float max_offset) {
  if (max_offset <= 0.0f) {
    return 0.0f;
  }
  if (step > 0.0f) {
    value = std::round(value / step) * step;
  }
  return std::clamp(value, 0.0f, max_offset);
}

}  // namespace

LobbyRoomListView::LobbyRoomListView(
    ClientContext& context,
    std::function<void(const protocol::RoomSummary&)> on_room_selected)
    : context_(context),
      on_room_selected_(std::move(on_room_selected)),
      menu_effects_(context, LobbyPointerConfig(),
                    constants::ui::kMenuHoverSfxPath,
                    constants::ui::kMenuClickSfxPath) {
  menu_effects_.Load();
}

void LobbyRoomListView::Update(engine::time::TimeDelta dt,
                               engine::input::InputManager& input) {
  context_.MenuBackground().Update(dt);
  UpdateScrollInput(input);
  menu_effects_.Update(dt, input, room_buttons_visible_);

  if (!header_frames_.empty() && header_animating_) {
    const float max_elapsed = static_cast<float>(header_frames_.size() - 1) *
                              constants::ui::Lobby::kHeaderFrameDuration;
    header_elapsed_ += dt.as_seconds();
    if (header_elapsed_ >= max_elapsed) {
      header_elapsed_ = max_elapsed;
      header_animating_ = false;
    }
  }

  if (!room_frames_.empty() && room_frame_animating_) {
    const float max_elapsed = static_cast<float>(room_frames_.size() - 1) *
                              constants::ui::Lobby::kRoomFrameDuration;
    room_frame_elapsed_ += dt.as_seconds();
    if (room_frame_elapsed_ >= max_elapsed) {
      room_frame_elapsed_ = max_elapsed;
      room_frame_animating_ = false;
    }
    const std::size_t frame_index = FrameIndex(
        room_frame_elapsed_, constants::ui::Lobby::kRoomFrameDuration,
        room_frames_.size());
    if (frame_index != room_frame_index_) {
      room_frame_index_ = frame_index;
      ApplyFrameTexture(room_buttons_, room_frames_[room_frame_index_]);
    }
  }

  for (auto& button : room_buttons_visible_) {
    if (button) {
      button->Update(dt, input);
    }
  }
}

void LobbyRoomListView::Layout(const engine::math::Vector2f& window_size) {
  const float width = window_size.x;
  const float margin = constants::ui::Lobby::kPanelMargin;
  const float available_width = std::max(0.0f, width - margin * 2.0f);

  title_anchor_ = {width * 0.5f, constants::ui::Lobby::kHeaderTopPadding};

  const float header_width = std::min(
      available_width, constants::ui::Lobby::kHeaderDecorationMaxWidth);
  const float header_height = constants::ui::Lobby::kHeaderDecorationHeight;
  const float header_x = (width - header_width) * 0.5f;
  const float header_y = title_anchor_.y +
                         constants::ui::Lobby::kHeaderTitleFontSize +
                         constants::ui::Lobby::kHeaderDecorationSpacing;
  header_frame_rect_ = {header_x, header_y, header_width, header_height};
  const float header_bottom =
      header_frame_rect_.top_left_y_ + header_frame_rect_.height_;
  const float create_button_y =
      header_bottom + constants::ui::Lobby::kHeaderDecorationBottomSpacing +
      constants::ui::Lobby::kCreateButtonOffsetY;

  const auto room_frame_size = TextureSizeOrFallback(
      room_frames_, constants::ui::Lobby::kRoomFrameDefaultWidth,
      constants::ui::Lobby::kRoomFrameDefaultHeight);
  const float available_room_width =
      std::max(0.0f, width - constants::ui::Lobby::kRoomListLeftPadding * 2.0f);
  const float room_width =
      std::min(available_room_width, constants::ui::Lobby::kRoomFrameMaxWidth);
  const float room_scale =
      room_frame_size.x > 0.0f ? room_width / room_frame_size.x : 1.0f;
  room_frame_draw_size_ = {room_width, room_frame_size.y * room_scale};
  room_list_origin_ = {constants::ui::Lobby::kRoomListLeftPadding,
                       create_button_y +
                           constants::ui::Lobby::kCreateButtonHeight +
                           constants::ui::Lobby::kRoomListSpacing};

  scroll_step_ =
      room_frame_draw_size_.y + constants::ui::Lobby::kRoomListSpacing;
  const float list_bottom = window_size.y -
                            constants::ui::Lobby::kBackButtonBottomPadding -
                            constants::ui::Lobby::kBackButtonHeight -
                            constants::ui::Lobby::kRoomListBottomPadding;
  const float list_available_height =
      std::max(0.0f, list_bottom - room_list_origin_.y);
  std::size_t max_visible_rooms = 0;
  if (scroll_step_ > 0.0f && list_available_height > 0.0f) {
    max_visible_rooms = static_cast<std::size_t>(
        (list_available_height + constants::ui::Lobby::kRoomListSpacing) /
        scroll_step_);
    if (max_visible_rooms == 0 && !room_buttons_.empty()) {
      max_visible_rooms = 1;
    }
  }
  float viewport_height = 0.0f;
  if (max_visible_rooms > 0) {
    viewport_height = static_cast<float>(max_visible_rooms) * scroll_step_ -
                      constants::ui::Lobby::kRoomListSpacing;
    viewport_height = std::min(viewport_height, list_available_height);
  }
  room_list_viewport_ = {room_list_origin_.x, room_list_origin_.y,
                         room_frame_draw_size_.x, viewport_height};
  room_list_content_height_ =
      RoomListContentHeight(room_buttons_.size(), room_frame_draw_size_.y,
                            constants::ui::Lobby::kRoomListSpacing);
  scroll_max_offset_ =
      std::max(0.0f, room_list_content_height_ - room_list_viewport_.height_);
  scroll_offset_ =
      SnapScrollOffset(scroll_offset_, scroll_step_, scroll_max_offset_);

  scroll_track_scale_ = constants::ui::Lobby::kScrollBarScale;
  float track_width = 0.0f;
  if (scroll_track_end_texture_) {
    const auto size = scroll_track_end_texture_->GetSize();
    if (size.x > 0) {
      track_width = static_cast<float>(size.x) * scroll_track_scale_;
    }
    if (size.y > 0 && room_list_viewport_.height_ > 0.0f) {
      const float scaled_end_height =
          static_cast<float>(size.y) * scroll_track_scale_;
      if (scaled_end_height * 2.0f > room_list_viewport_.height_) {
        scroll_track_scale_ =
            room_list_viewport_.height_ / (2.0f * static_cast<float>(size.y));
        track_width = static_cast<float>(size.x) * scroll_track_scale_;
      }
    }
  }
  if (track_width <= 0.0f && scroll_handle_texture_) {
    const auto size = scroll_handle_texture_->GetSize();
    if (size.x > 0) {
      track_width = static_cast<float>(size.x) * scroll_track_scale_;
    }
  }
  if (track_width <= 0.0f) {
    track_width = room_frame_draw_size_.x * 0.1f;
  }
  float scroll_x = room_list_origin_.x + room_frame_draw_size_.x +
                   constants::ui::Lobby::kPointerSpacing +
                   constants::ui::Lobby::kScrollBarGap;
  const float max_x =
      window_size.x - constants::ui::Lobby::kPanelMargin - track_width;
  scroll_x = std::min(scroll_x, max_x);
  scroll_x = std::max(0.0f, scroll_x);
  scroll_track_rect_ = {scroll_x, room_list_viewport_.top_left_y_, track_width,
                        room_list_viewport_.height_};

  ApplyRoomLayout();
  UpdateScrollHandleRect();
}

void LobbyRoomListView::ApplyRoomLayout() {
  room_buttons_visible_.assign(room_buttons_.size(), nullptr);
  if (room_buttons_.empty()) {
    return;
  }
  if (scroll_step_ <= 0.0f || room_list_viewport_.height_ <= 0.0f) {
    return;
  }
  const float view_top = room_list_viewport_.top_left_y_;
  const float view_bottom = view_top + room_list_viewport_.height_;
  for (std::size_t i = 0; i < room_buttons_.size(); ++i) {
    auto& button = room_buttons_[i];
    if (!button) {
      continue;
    }
    const float y = room_list_origin_.y + static_cast<float>(i) * scroll_step_ -
                    scroll_offset_;
    button->SetPosition({room_list_origin_.x, y});
    button->SetSize(room_frame_draw_size_);
    const float bottom = y + room_frame_draw_size_.y;
    if (y >= view_top && bottom <= view_bottom) {
      room_buttons_visible_[i] = button;
    }
  }
}

void LobbyRoomListView::UpdateScrollInput(engine::input::InputManager& input) {
  UpdateScrollHandleRect();
  const auto mouse_pos = input.GetMousePosition();
  const bool left_down =
      input.IsMouseButtonDown(engine::input::MouseButton::kLeft);

  if (!left_down) {
    scroll_dragging_ = false;
  }

  if (scroll_max_offset_ <= 0.0f || scroll_track_rect_.height_ <= 0.0f) {
    scroll_offset_ = 0.0f;
    scroll_wheel_accumulator_ = 0.0f;
    scroll_dragging_ = false;
    was_left_down_ = left_down;
    ApplyRoomLayout();
    UpdateScrollHandleRect();
    return;
  }

  const auto offset_from_handle_top = [this](float handle_top) {
    const float handle_range =
        std::max(0.0f, scroll_track_rect_.height_ - scroll_handle_height_);
    if (handle_range <= 0.0f) {
      return 0.0f;
    }
    const float min_top = scroll_track_rect_.top_left_y_;
    const float max_top = scroll_track_rect_.top_left_y_ + handle_range;
    const float clamped = std::clamp(handle_top, min_top, max_top);
    const float ratio = (clamped - min_top) / handle_range;
    return SnapScrollOffset(ratio * scroll_max_offset_, scroll_step_,
                            scroll_max_offset_);
  };

  const float wheel_delta = input.GetMouseWheelDelta();
  const bool hover_scroll = room_list_viewport_.Contains(mouse_pos) ||
                            scroll_track_rect_.Contains(mouse_pos);
  if (!scroll_dragging_ && hover_scroll && wheel_delta != 0.0f &&
      scroll_step_ > 0.0f) {
    scroll_wheel_accumulator_ += wheel_delta;
    const int wheel_steps = static_cast<int>(scroll_wheel_accumulator_);
    if (wheel_steps != 0) {
      scroll_offset_ = SnapScrollOffset(
          scroll_offset_ - static_cast<float>(wheel_steps) * scroll_step_,
          scroll_step_, scroll_max_offset_);
      scroll_wheel_accumulator_ -= static_cast<float>(wheel_steps);
    }
  }

  if (left_down && !was_left_down_) {
    if (scroll_handle_rect_.Contains(mouse_pos)) {
      scroll_dragging_ = true;
      scroll_drag_offset_ = mouse_pos.y - scroll_handle_rect_.top_left_y_;
    } else if (scroll_track_rect_.Contains(mouse_pos)) {
      scroll_offset_ = offset_from_handle_top(
          mouse_pos.y - scroll_handle_rect_.height_ * 0.5f);
    }
  }

  if (scroll_dragging_) {
    scroll_offset_ = offset_from_handle_top(mouse_pos.y - scroll_drag_offset_);
  }

  was_left_down_ = left_down;
  ApplyRoomLayout();
  UpdateScrollHandleRect();
}

void LobbyRoomListView::UpdateScrollHandleRect() {
  scroll_handle_rect_ = {};
  scroll_handle_height_ = 0.0f;
  if (scroll_track_rect_.height_ <= 0.0f || scroll_track_rect_.width_ <= 0.0f) {
    return;
  }

  float handle_width = scroll_track_rect_.width_;
  float handle_height = scroll_track_rect_.height_;
  if (scroll_handle_texture_) {
    const auto size = scroll_handle_texture_->GetSize();
    if (size.x > 0) {
      handle_width = static_cast<float>(size.x) * scroll_track_scale_;
    }
    if (size.y > 0) {
      handle_height = static_cast<float>(size.y) * scroll_track_scale_;
    }
  }

  handle_height = std::min(handle_height, scroll_track_rect_.height_);
  scroll_handle_height_ = handle_height;

  const float handle_range =
      std::max(0.0f, scroll_track_rect_.height_ - handle_height);
  float handle_y = scroll_track_rect_.top_left_y_;
  if (scroll_max_offset_ > 0.0f && handle_range > 0.0f) {
    handle_y += (scroll_offset_ / scroll_max_offset_) * handle_range;
  }
  const float handle_x = scroll_track_rect_.top_left_x_ +
                         (scroll_track_rect_.width_ - handle_width) * 0.5f;
  scroll_handle_rect_ = {handle_x, handle_y, handle_width, handle_height};
}

void LobbyRoomListView::DrawScrollBar(
    engine::render::Renderer2D& renderer) const {
  if (scroll_track_rect_.height_ <= 0.0f || scroll_track_rect_.width_ <= 0.0f) {
    return;
  }

  const float track_x = scroll_track_rect_.top_left_x_;
  const float track_y = scroll_track_rect_.top_left_y_;
  const float track_height = scroll_track_rect_.height_;

  float end_height = 0.0f;
  float end_width = 0.0f;
  if (scroll_track_end_texture_) {
    const auto end_size = scroll_track_end_texture_->GetSize();
    if (end_size.x > 0 && end_size.y > 0) {
      end_width = static_cast<float>(end_size.x) * scroll_track_scale_;
      end_height = static_cast<float>(end_size.y) * scroll_track_scale_;
      const float end_x =
          track_x + (scroll_track_rect_.width_ - end_width) * 0.5f;

      engine::render::SpriteDrawParams top_params;
      top_params.position = {end_x, track_y};
      top_params.scale = {scroll_track_scale_, scroll_track_scale_};
      renderer.DrawTexture(*scroll_track_end_texture_, top_params);

      const float bottom_y = track_y + track_height - end_height;
      engine::render::SpriteDrawParams bottom_params;
      bottom_params.scale = {scroll_track_scale_, scroll_track_scale_};
      bottom_params.origin = {end_width * 0.5f, end_height * 0.5f};
      bottom_params.position = {end_x + end_width * 0.5f,
                                bottom_y + end_height * 0.5f};
      bottom_params.source = engine::math::RectF{
          static_cast<float>(end_size.x), 0.0f, -static_cast<float>(end_size.x),
          static_cast<float>(end_size.y)};
      bottom_params.rotation = 180.0f;
      renderer.DrawTexture(*scroll_track_end_texture_, bottom_params);
    }
  }

  if (scroll_track_mid_texture_) {
    const auto mid_size = scroll_track_mid_texture_->GetSize();
    if (mid_size.x > 0 && mid_size.y > 0) {
      const float mid_width =
          static_cast<float>(mid_size.x) * scroll_track_scale_;
      const float mid_height =
          static_cast<float>(mid_size.y) * scroll_track_scale_;
      const float mid_x =
          track_x + (scroll_track_rect_.width_ - mid_width) * 0.5f;
      const float mid_start = track_y + end_height;
      const float mid_end = track_y + track_height - end_height;
      float y = mid_start;
      while (y + mid_height <= mid_end) {
        engine::render::SpriteDrawParams params;
        params.position = {mid_x, y};
        params.scale = {scroll_track_scale_, scroll_track_scale_};
        renderer.DrawTexture(*scroll_track_mid_texture_, params);
        y += mid_height;
      }
      const float remaining = mid_end - y;
      if (remaining > 0.0f) {
        engine::render::SpriteDrawParams params;
        params.position = {mid_x, y};
        params.scale = {scroll_track_scale_,
                        remaining / static_cast<float>(mid_size.y)};
        renderer.DrawTexture(*scroll_track_mid_texture_, params);
      }
    }
  }

  if (scroll_handle_texture_) {
    const auto handle_size = scroll_handle_texture_->GetSize();
    if (handle_size.x > 0 && handle_size.y > 0 &&
        scroll_handle_rect_.height_ > 0.0f) {
      engine::render::SpriteDrawParams params;
      params.position = {scroll_handle_rect_.top_left_x_,
                         scroll_handle_rect_.top_left_y_};
      params.scale = {
          scroll_handle_rect_.width_ / static_cast<float>(handle_size.x),
          scroll_handle_rect_.height_ / static_cast<float>(handle_size.y)};
      renderer.DrawTexture(*scroll_handle_texture_, params);
    }
  }
}

void LobbyRoomListView::DrawBackground() const {
  context_.MenuBackground().Draw(context_.Window());
}

void LobbyRoomListView::DrawForeground(engine::render::Renderer2D& renderer,
                                       std::string_view status_text) const {
  const std::string title = "Select Room";
  renderer.SetFont(std::string(constants::ui::kTitleFont));
  const auto title_size =
      renderer.MeasureText(title, constants::ui::Lobby::kHeaderTitleFontSize);
  renderer.DrawText(title,
                    {title_anchor_.x - title_size.x * 0.5f, title_anchor_.y},
                    constants::ui::Lobby::kHeaderTitleFontSize,
                    engine::render::Color::White());

  if (!header_frames_.empty()) {
    const std::size_t frame_index =
        FrameIndex(header_elapsed_, constants::ui::Lobby::kHeaderFrameDuration,
                   header_frames_.size());
    auto texture = header_frames_[frame_index];
    if (texture) {
      const auto tex_size = texture->GetSize();
      if (tex_size.x > 0 && tex_size.y > 0) {
        const float scale = std::min(
            header_frame_rect_.width_ / static_cast<float>(tex_size.x),
            header_frame_rect_.height_ / static_cast<float>(tex_size.y));
        if (scale > 0.0f) {
          const float draw_width = static_cast<float>(tex_size.x) * scale;
          const float draw_height = static_cast<float>(tex_size.y) * scale;
          const float x = header_frame_rect_.top_left_x_ +
                          (header_frame_rect_.width_ - draw_width) * 0.5f;
          const float y = header_frame_rect_.top_left_y_ +
                          (header_frame_rect_.height_ - draw_height) * 0.5f;
          engine::render::SpriteDrawParams params;
          params.position = {x, y};
          params.scale = {scale, scale};
          renderer.DrawTexture(*texture, params);
        }
      }
    }
  }

  const float base_font_size =
      room_frame_draw_size_.y * constants::ui::Lobby::kRoomTextScale;
  const float min_font_size =
      room_frame_draw_size_.y * constants::ui::Lobby::kRoomTextMinScale;
  const std::size_t button_count =
      std::min(room_buttons_.size(), room_buttons_visible_.size());
  for (std::size_t i = 0; i < button_count; ++i) {
    auto& button = room_buttons_visible_[i];
    if (!button) {
      continue;
    }
    if (i < room_entries_.size()) {
      const auto& entry = room_entries_[i];
      if (entry.area_texture) {
        const auto tex_size = entry.area_texture->GetSize();
        if (tex_size.x > 0 && tex_size.y > 0) {
          const auto pos = button->GetPosition();
          const auto size = button->GetSize();
          engine::render::SpriteDrawParams params;
          params.position = {pos.x,
                             pos.y + constants::ui::Lobby::kRoomAreaOffsetY};
          params.scale = {size.x / static_cast<float>(tex_size.x),
                          size.y / static_cast<float>(tex_size.y)};
          renderer.DrawTexture(*entry.area_texture, params);
        }
      }
    }
    button->Draw(renderer);
    if (i >= room_entries_.size()) {
      continue;
    }
    const auto& entry = room_entries_[i];
    const auto pos = button->GetPosition();
    const auto size = button->GetSize();
    const float center_y =
        pos.y + size.y * 0.5f + constants::ui::Lobby::kRoomTextOffsetY;
    renderer.SetFont(std::string(constants::ui::kTitleFont));
    const auto left_size =
        renderer.MeasureText(entry.left_text, base_font_size);
    const float padding = constants::ui::Lobby::kRoomTextPadding;
    const float gap = constants::ui::Lobby::kRoomTextGap;
    const float max_right_width =
        std::max(0.0f, size.x - padding * 2.0f - left_size.x - gap);
    float right_font_size = base_font_size;
    renderer.SetFont(std::string(constants::ui::kTitleFont));
    auto right_size = renderer.MeasureText(entry.right_text, right_font_size);
    if (max_right_width > 0.0f && right_size.x > max_right_width) {
      const float scale = max_right_width / right_size.x;
      right_font_size = std::max(min_font_size, base_font_size * scale);
      right_size = renderer.MeasureText(entry.right_text, right_font_size);
    }
    const float left_bound = pos.x + padding + left_size.x + gap;
    const float right_bound = pos.x + size.x - padding - right_size.x - gap;
    float center_font_size = base_font_size;
    renderer.SetFont(std::string(constants::ui::kBodyFont));
    auto center_size =
        renderer.MeasureText(entry.center_text, center_font_size);
    const float max_center_width = std::max(0.0f, right_bound - left_bound);
    if (max_center_width > 0.0f && center_size.x > max_center_width) {
      const float scale = max_center_width / center_size.x;
      center_font_size = std::max(min_font_size, base_font_size * scale);
      center_size = renderer.MeasureText(entry.center_text, center_font_size);
    }
    const float left_y = center_y - left_size.y * 0.5f;
    renderer.SetFont(std::string(constants::ui::kTitleFont));
    renderer.DrawText(entry.left_text, {pos.x + padding, left_y},
                      base_font_size, constants::ui::Lobby::kRoomTextColor);
    float center_x = pos.x + (size.x - center_size.x) * 0.5f;
    if (max_center_width > 0.0f) {
      center_x = std::clamp(center_x, left_bound, right_bound - center_size.x);
    }
    const float center_y_pos = center_y - center_size.y * 0.5f;
    renderer.SetFont(std::string(constants::ui::kBodyFont));
    renderer.DrawText(entry.center_text, {center_x, center_y_pos},
                      center_font_size, constants::ui::Lobby::kRoomTextColor);
    const float right_x = pos.x + size.x - padding - right_size.x;
    const float right_y = center_y - right_size.y * 0.5f;
    renderer.SetFont(std::string(constants::ui::kTitleFont));
    renderer.DrawText(entry.right_text, {right_x, right_y}, right_font_size,
                      constants::ui::Lobby::kRoomTextColor);
  }

  DrawScrollBar(renderer);
  menu_effects_.DrawPointers(renderer, room_buttons_visible_);

  static_cast<void>(status_text);
}

void LobbyRoomListView::Draw(engine::render::Renderer2D& renderer,
                             std::string_view status_text) const {
  DrawBackground();
  DrawForeground(renderer, status_text);
}

void LobbyRoomListView::RefreshRooms(
    const std::vector<protocol::RoomSummary>& rooms,
    ClientAssetManager& assets) {
  LoadFrameSequence(assets, constants::ui::Lobby::kHeaderFramePrefix,
                    constants::ui::Lobby::kHeaderFrameExtension,
                    constants::ui::Lobby::kHeaderFrameCount, header_frames_);
  LoadFrameSequence(assets, constants::ui::Lobby::kRoomFramePrefix,
                    constants::ui::Lobby::kRoomFrameExtension,
                    constants::ui::Lobby::kRoomFrameCount, room_frames_);
  LoadAreaTextures(assets, area_textures_);
  if (!scroll_handle_texture_) {
    scroll_handle_texture_ =
        assets.GetTexture(constants::ui::Lobby::kScrollBarHandlePath);
  }
  if (!scroll_track_end_texture_) {
    scroll_track_end_texture_ =
        assets.GetTexture(constants::ui::Lobby::kScrollBarEndPath);
  }
  if (!scroll_track_mid_texture_) {
    scroll_track_mid_texture_ =
        assets.GetTexture(constants::ui::Lobby::kScrollBarMidPath);
  }

  const auto hash = HashRooms(rooms);
  if (hash == last_rooms_hash_) {
    return;
  }
  last_rooms_hash_ = hash;
  room_buttons_.clear();
  room_buttons_visible_.clear();
  room_entries_.clear();

  if (!room_frames_.empty() && room_frame_index_ >= room_frames_.size()) {
    room_frame_index_ = 0;
  }
  const auto room_texture =
      room_frames_.empty() ? nullptr : room_frames_[room_frame_index_];
  const auto tint = engine::render::Color::White();

  for (const auto& room : rooms) {
    RoomEntry entry{};
    entry.room = room;
    entry.left_text = ExtractRoomId(room.room_code);
    entry.center_text = BuildRoomCenterText(room);
    entry.right_text = BuildRoomRightText(room);
    if (!area_textures_.empty()) {
      const auto index =
          std::hash<std::string>{}(room.room_code) % area_textures_.size();
      entry.area_texture = area_textures_[index];
    }
    auto button = std::make_shared<engine::ui::Button>(
        engine::math::Vector2f{},
        engine::math::Vector2f{0.0f, constants::ui::Lobby::kRoomButtonHeight},
        "", menu_effects_.WrapClick([this, room]() {
          if (on_room_selected_) {
            on_room_selected_(room);
          }
        }));
    if (room_texture) {
      button->SetTexture(room_texture);
      button->SetColors(tint, tint, tint);
    }
    button->SetTextColor(constants::ui::Lobby::kRoomTextColor);
    button->SetTextScale(constants::ui::Lobby::kRoomTextScale);
    room_buttons_.push_back(button);
    room_entries_.push_back(std::move(entry));
  }

  ApplyFrameTexture(room_buttons_, room_texture);
}

}  // namespace client
