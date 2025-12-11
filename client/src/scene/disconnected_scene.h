#ifndef CLIENT_SCENE_DISCONNECTED_SCENE_H_
#define CLIENT_SCENE_DISCONNECTED_SCENE_H_

#include <string>
#include "scene.h"

namespace client {

class Application;

class DisconnectedScene : public Scene {
 public:
  explicit DisconnectedScene(Application& app, std::string reason);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  Application& app_;
  std::string reason_;
};

}  // namespace client

#endif  // CLIENT_SCENE_DISCONNECTED_SCENE_H_
