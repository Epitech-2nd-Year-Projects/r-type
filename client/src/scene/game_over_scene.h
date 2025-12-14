#ifndef CLIENT_SCENE_GAME_OVER_SCENE_H_
#define CLIENT_SCENE_GAME_OVER_SCENE_H_

#include <memory>

#include "scene.h"
#include "engine/ui/canvas.h"
#include "engine/ui/text.h"

namespace client {

class Application;

class GameOverScene : public Scene {
 public:
  explicit GameOverScene(Application& app);

  void Update(engine::time::TimeDelta dt) override;
 void Draw(engine::render::Renderer2D& renderer) override;

 private:
  Application& app_;
  engine::ui::Canvas canvas_;
  std::shared_ptr<engine::ui::TextElement> title_;
  std::shared_ptr<engine::ui::TextElement> prompt_;
};

}  // namespace client

#endif  // CLIENT_SCENE_GAME_OVER_SCENE_H_
