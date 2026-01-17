#ifndef CLIENT_SCENE_PROFILE_SCENE_H_
#define CLIENT_SCENE_PROFILE_SCENE_H_

#include <memory>
#include <string>

#include "scene.h"
#include "engine/ui/button.h"
#include "engine/ui/canvas.h"
#include "engine/ui/text.h"
#include "engine/ui/text_input.h"
#include "engine/ui/widget.h"
#include "ui/avatar_renderer.h"
#include "ui/menu_effects.h"

namespace client {

class ClientContext;

/**
 * @brief Scene for editing player profile (nickname and viewing stats)
 */
class ProfileScene : public Scene {
 public:
  explicit ProfileScene(ClientContext& context);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;
  void DrawBackground(engine::render::Renderer2D& renderer) override;
  void DrawForeground(engine::render::Renderer2D& renderer) override;
  bool IsInputCaptured() const override { return text_input_focused_; }

 private:
  void LayoutUi(engine::render::Renderer2D& renderer);
  void DrawTitleFleur(engine::render::Renderer2D& renderer);
  void DrawStatsBorder(engine::render::Renderer2D& renderer);
  void DrawInputBackground(engine::render::Renderer2D& renderer);
  void SaveAndClose();
  void FormatPlaytime(std::uint64_t seconds, std::string& out) const;
  void SelectPrevAvatar();
  void SelectNextAvatar();
  void SelectPrevColor();
  void SelectNextColor();

  ClientContext& context_;
  engine::ui::Canvas canvas_;

  engine::math::RectF title_rect_{};
  std::vector<std::shared_ptr<engine::render::Texture2D>> fleur_frames_;
  engine::math::RectF fleur_rect_{};
  float fleur_elapsed_{0.0f};
  bool fleur_animating_{true};
  std::shared_ptr<engine::render::Texture2D> stats_border_texture_;
  engine::math::RectF stats_rect_{};
  std::shared_ptr<engine::render::Texture2D> input_bg_texture_;
  std::shared_ptr<engine::render::Texture2D> arrow_left_texture_;
  std::shared_ptr<engine::render::Texture2D> arrow_right_texture_;
  std::shared_ptr<engine::ui::TextElement> nickname_label_;
  std::shared_ptr<engine::ui::TextInput> nickname_input_;
  std::shared_ptr<engine::ui::TextElement> stats_header_;
  std::shared_ptr<engine::ui::TextElement> playtime_text_;
  std::shared_ptr<engine::ui::TextElement> deaths_text_;
  std::shared_ptr<engine::ui::TextElement> highest_score_text_;
  std::shared_ptr<engine::ui::TextElement> games_played_text_;
  std::shared_ptr<engine::ui::Button> save_button_;
  std::shared_ptr<engine::ui::Button> back_button_;

  std::vector<std::shared_ptr<engine::ui::Widget>> ui_elements_;
  std::vector<std::shared_ptr<engine::ui::Button>> buttons_;
  bool text_input_focused_{false};

  std::unique_ptr<ui::AvatarRenderer> avatar_renderer_;
  std::uint8_t selected_avatar_{0};
  std::shared_ptr<engine::ui::Button> avatar_left_button_;
  std::shared_ptr<engine::ui::Button> avatar_right_button_;
  engine::math::Vector2f avatar_position_{0.0f, 0.0f};
  std::uint8_t selected_color_index_{0};
  std::shared_ptr<engine::ui::Button> color_left_button_;
  std::shared_ptr<engine::ui::Button> color_right_button_;
  engine::math::Vector2f color_preview_position_{0.0f, 0.0f};

  ui::MenuEffects menu_effects_;
};

}  // namespace client

#endif  // CLIENT_SCENE_PROFILE_SCENE_H_
