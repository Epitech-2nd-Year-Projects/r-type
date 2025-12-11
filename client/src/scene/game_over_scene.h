#ifndef CLIENT_SCENE_GAME_OVER_SCENE_H_
#define CLIENT_SCENE_GAME_OVER_SCENE_H_

#include "scene.h"

namespace client {

class Application;

class GameOverScene : public Scene {
 public:
  explicit GameOverScene(Application& app);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  Application& app_;
};

}  // namespace client

#endif  // CLIENT_SCENE_GAME_OVER_SCENE_H_
