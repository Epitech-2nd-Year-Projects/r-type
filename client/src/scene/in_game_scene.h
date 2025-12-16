#ifndef CLIENT_SCENE_IN_GAME_SCENE_H_
#define CLIENT_SCENE_IN_GAME_SCENE_H_

#include "hud_overlay.h"
#include "scene.h"

namespace client {

class Application;

class InGameScene : public Scene {
 public:
  explicit InGameScene(Application& app);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  Application& app_;
  HudOverlay hud_;
  bool is_ready_{false};
  bool toggle_pressed_{false};
  bool pause_pressed_{false};
};

}  // namespace client

#endif  // CLIENT_SCENE_IN_GAME_SCENE_H_
