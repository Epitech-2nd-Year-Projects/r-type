#ifndef CLIENT_SCENE_SPLASH_SCENE_H_
#define CLIENT_SCENE_SPLASH_SCENE_H_

#include <memory>
#include <string>

#include "scene.h"
#include "engine/math/rect.h"
#include "ui/menu_background.h"

namespace engine::render {
class Texture2D;
}

namespace engine::input {
class InputManager;
}

namespace client {

class ClientContext;

/**
 * @brief Splash screen scene
 */
class SplashScene : public Scene {
 public:
  /**
   * @brief Create splash scene
   * @param context Client context reference
   */
  explicit SplashScene(ClientContext& context);

  /**
   * @brief Update splash scene
   * @param dt Frame delta
   */
  void Update(engine::time::TimeDelta dt) override;

  /**
   * @brief Draw splash scene
   * @param renderer Renderer instance
   */
  void Draw(engine::render::Renderer2D& renderer) override;

  /**
   * @brief Draw splash background
   * @param renderer Renderer instance
   */
  void DrawBackground(engine::render::Renderer2D& renderer) override;

  /**
   * @brief Draw splash foreground
   * @param renderer Renderer instance
   */
  void DrawForeground(engine::render::Renderer2D& renderer) override;

 private:
  bool IsAnyInputDown(engine::input::InputManager& input) const;
  engine::math::RectF ComputeTitleRect(float width, float height) const;
  void DrawLogo(engine::render::Renderer2D& renderer);
  void DrawPrompt(engine::render::Renderer2D& renderer);
  void DrawCopyright(engine::render::Renderer2D& renderer);
  float TransitionAlpha() const;
  void TriggerTransition();

  ClientContext& context_;
  ui::MenuBackground background_;
  std::string prompt_text_;
  std::string copyright_text_;
  std::string transition_sfx_path_;
  std::shared_ptr<engine::render::Texture2D> title_texture_;
  engine::math::RectF title_rect_{};
  engine::time::TimeDelta transition_elapsed_{engine::time::TimeDelta::zero()};
  bool input_was_down_{false};
  bool transition_started_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_SPLASH_SCENE_H_
