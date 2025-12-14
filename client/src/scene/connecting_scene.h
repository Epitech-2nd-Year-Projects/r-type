#ifndef CLIENT_SCENE_CONNECTING_SCENE_H_
#define CLIENT_SCENE_CONNECTING_SCENE_H_

#include <memory>

#include "scene.h"
#include "engine/ui/canvas.h"
#include "engine/ui/text.h"

namespace client {

class Application;

class ConnectingScene : public Scene {
 public:
  explicit ConnectingScene(Application& app);

  void Update(engine::time::TimeDelta dt) override;
 void Draw(engine::render::Renderer2D& renderer) override;

 private:
  Application& app_;
  engine::ui::Canvas canvas_;
  std::shared_ptr<engine::ui::TextElement> status_text_;
};

}  // namespace client

#endif  // CLIENT_SCENE_CONNECTING_SCENE_H_
