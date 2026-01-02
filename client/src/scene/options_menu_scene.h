#ifndef CLIENT_SCENE_OPTIONS_MENU_SCENE_H_
#define CLIENT_SCENE_OPTIONS_MENU_SCENE_H_

#include <memory>
#include <vector>

#include "engine/ui/button.h"
#include "engine/math/rect.h"
#include "engine/ui/canvas.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"
#include "scene.h"
#include "ui/menu_effects.h"

namespace client {

class ClientContext;

/**
 * @brief Options menu scene
 *
 * Shows option categories and back entry
 */
class OptionsMenuScene : public Scene {
 public:
  /**
   * @brief Build the options menu scene
   * @param context Client context reference
   */
  explicit OptionsMenuScene(ClientContext& context);

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
   * @brief Draw options background
   * @param renderer Renderer instance
   */
  void DrawBackground(engine::render::Renderer2D& renderer) override;

  /**
   * @brief Draw options foreground
   * @param renderer Renderer instance
   */
  void DrawForeground(engine::render::Renderer2D& renderer) override;

 private:
  void LayoutUi(engine::render::Renderer2D& renderer);
  void DrawWarning(engine::render::Renderer2D& renderer);

  ClientContext& context_;
  engine::ui::Canvas canvas_;
  std::vector<std::shared_ptr<engine::ui::Button>> buttons_;
  std::vector<std::shared_ptr<engine::render::Texture2D>> warning_frames_;
  engine::math::RectF warning_rect_{};
  float warning_elapsed_{0.0f};
  bool warning_animating_{true};
  ui::MenuEffects menu_effects_;
};

}  // namespace client

#endif  // CLIENT_SCENE_OPTIONS_MENU_SCENE_H_
