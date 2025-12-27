#ifndef CLIENT_SCENE_GAME_OVER_SCENE_H_
#define CLIENT_SCENE_GAME_OVER_SCENE_H_

#include <cstdint>
#include <memory>

#include "scene.h"
#include "engine/ui/canvas.h"
#include "engine/ui/text.h"

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

 private:
  void UpdateMenuVisuals();

  ClientContext& context_;
  Stats stats_;
  engine::ui::Canvas canvas_;
  std::shared_ptr<engine::ui::TextElement> title_;
  std::shared_ptr<engine::ui::TextElement> score_text_;
  std::shared_ptr<engine::ui::TextElement> wave_level_text_;
  std::shared_ptr<engine::ui::TextElement> menu_main_exit_;
};

}  // namespace client

#endif  // CLIENT_SCENE_GAME_OVER_SCENE_H_
