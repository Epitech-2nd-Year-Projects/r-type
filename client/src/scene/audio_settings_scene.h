#ifndef CLIENT_SCENE_AUDIO_SETTINGS_SCENE_H_
#define CLIENT_SCENE_AUDIO_SETTINGS_SCENE_H_

#include <memory>
#include <string>
#include <vector>

#include "engine/math/rect.h"
#include "engine/ui/button.h"
#include "engine/ui/canvas.h"
#include "scene.h"
#include "ui/menu_effects.h"

namespace client {

class ClientContext;

/**
 * @brief Audio settings scene
 *
 * Provides volume sliders and reset/back actions
 */
class AudioSettingsScene : public Scene {
 public:
  /**
   * @brief Build the audio settings scene
   * @param context Client context reference
   */
  explicit AudioSettingsScene(ClientContext& context);

  /**
   * @brief Update scene state
   * @param dt Frame time delta
   */
  void Update(engine::time::TimeDelta dt) override;

  /**
   * @brief Draw scene visuals
   * @param renderer Renderer instance
   */
  void Draw(engine::render::Renderer2D& renderer) override;

  /**
   * @brief Draw background
   */
  void DrawBackground(engine::render::Renderer2D& renderer) override;

  /**
   * @brief Draw foreground
   */
  void DrawForeground(engine::render::Renderer2D& renderer) override;

 private:
  struct VolumeSlider {
    std::string label;
    int value{0};
    engine::math::RectF row_rect{};
    engine::math::RectF track_rect{};
    engine::math::RectF handle_rect{};
    float label_width{0.0f};
    float value_width{0.0f};
    std::shared_ptr<engine::ui::Button> handle_button;
    bool dragging{false};
  };

  void LayoutUi(engine::render::Renderer2D& renderer);
  void DrawWarning(engine::render::Renderer2D& renderer);
  void DrawSliders(engine::render::Renderer2D& renderer);
  void UpdateSliderLayout(engine::render::Renderer2D& renderer);
  void ApplyVolumes();
  void ResetDefaults();

  ClientContext& context_;
  engine::ui::Canvas canvas_;
  std::vector<std::shared_ptr<engine::ui::Button>> controls_;
  std::vector<std::shared_ptr<engine::ui::Button>> pointer_buttons_;
  std::vector<VolumeSlider> sliders_;
  std::shared_ptr<engine::ui::Button> reset_button_;
  std::shared_ptr<engine::ui::Button> reset_pointer_button_;
  std::shared_ptr<engine::ui::Button> back_button_;
  std::vector<std::shared_ptr<engine::render::Texture2D>> warning_frames_;
  std::shared_ptr<engine::render::Texture2D> bar_texture_;
  std::shared_ptr<engine::render::Texture2D> bar_end_texture_;
  std::shared_ptr<engine::render::Texture2D> handle_texture_;
  float max_label_width_{0.0f};
  engine::math::RectF warning_rect_{};
  float warning_elapsed_{0.0f};
  bool warning_animating_{true};
  bool was_left_down_{false};
  ui::MenuEffects menu_effects_;
};

}  // namespace client

#endif  // CLIENT_SCENE_AUDIO_SETTINGS_SCENE_H_
