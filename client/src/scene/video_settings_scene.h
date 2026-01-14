#ifndef CLIENT_SCENE_VIDEO_SETTINGS_SCENE_H_
#define CLIENT_SCENE_VIDEO_SETTINGS_SCENE_H_

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
 * @brief Video settings scene
 *
 * Provides resolution and display settings with apply/reset/back actions
 */
class VideoSettingsScene : public Scene {
 public:
  /**
   * @brief Build the video settings scene
   * @param context Client context reference
   */
  explicit VideoSettingsScene(ClientContext& context);

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
  enum class SettingId { kResolution, kFullscreen, kVsync, kMaxFps };

  struct ResolutionOption {
    int width{0};
    int height{0};
    std::string label;
  };

  struct SettingRow {
    SettingId id;
    std::string label;
    std::vector<std::string> options;
    int index{0};
    engine::math::RectF row_rect{};
    std::shared_ptr<engine::ui::Button> button;
  };

  void LayoutUi(engine::render::Renderer2D& renderer);
  void DrawWarning(engine::render::Renderer2D& renderer);
  void DrawRows(engine::render::Renderer2D& renderer);
  void AdvanceSetting(SettingId id);
  void SyncRowsFromState();
  void ApplySettings();
  void ResetDefaults();
  int FindResolutionIndex(int width, int height) const;
  int FindFpsIndex(int target_fps) const;

  ClientContext& context_;
  engine::ui::Canvas canvas_;
  std::vector<std::shared_ptr<engine::ui::Button>> controls_;
  std::vector<std::shared_ptr<engine::ui::Button>> pointer_buttons_;
  std::vector<std::shared_ptr<engine::render::Texture2D>> warning_frames_;
  engine::math::RectF warning_rect_{};
  float warning_elapsed_{0.0f};
  bool warning_animating_{true};
  ui::MenuEffects menu_effects_;

  std::vector<ResolutionOption> resolutions_;
  std::vector<int> fps_options_;
  std::vector<SettingRow> rows_;
  std::shared_ptr<engine::ui::Button> apply_button_;
  std::shared_ptr<engine::ui::Button> reset_button_;
  std::shared_ptr<engine::ui::Button> reset_pointer_button_;
  std::shared_ptr<engine::ui::Button> back_button_;

  int pending_resolution_width_{0};
  int pending_resolution_height_{0};
  bool pending_fullscreen_{false};
  bool pending_vsync_{true};
  int pending_target_fps_{60};
};

}  // namespace client

#endif  // CLIENT_SCENE_VIDEO_SETTINGS_SCENE_H_
