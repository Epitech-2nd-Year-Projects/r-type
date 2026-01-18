#ifndef CLIENT_SCENE_GAME_OVER_SCENE_H_
#define CLIENT_SCENE_GAME_OVER_SCENE_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "engine/ui/button.h"
#include "engine/ui/canvas.h"
#include "engine/ui/text.h"
#include "scene.h"
#include "ui/menu_effects.h"

namespace client {

class ClientContext;

class GameOverScene : public Scene {
 public:
  struct Stats {
    std::uint32_t score{0};
    std::uint32_t wave{1};
  };

  explicit GameOverScene(ClientContext& context, const Stats& stats);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

  void DrawBackground(engine::render::Renderer2D& renderer) override;
  void DrawForeground(engine::render::Renderer2D& renderer) override;

 private:
  void LayoutUi(engine::render::Renderer2D& renderer);

  ClientContext& context_;
  Stats stats_;
  engine::ui::Canvas canvas_;
  std::shared_ptr<engine::ui::TextElement> title_;
  std::shared_ptr<engine::ui::TextElement> score_text_;
  std::shared_ptr<engine::ui::TextElement> wave_level_text_;
  std::shared_ptr<engine::ui::Button> menu_main_exit_button_;
  std::vector<std::shared_ptr<engine::ui::Button>> ui_elements_;
  ui::MenuEffects menu_effects_;
};

}  // namespace client

#endif  // CLIENT_SCENE_GAME_OVER_SCENE_H_
