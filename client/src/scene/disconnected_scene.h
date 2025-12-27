#ifndef CLIENT_SCENE_DISCONNECTED_SCENE_H_
#define CLIENT_SCENE_DISCONNECTED_SCENE_H_

#include <memory>
#include <string>

#include "scene.h"
#include "engine/ui/canvas.h"
#include "engine/ui/text.h"

namespace client {

class ClientContext;

class DisconnectedScene : public Scene {
 public:
  explicit DisconnectedScene(ClientContext& context, std::string reason);

  void Update(engine::time::TimeDelta dt) override;
  void Draw(engine::render::Renderer2D& renderer) override;

 private:
  ClientContext& context_;
  std::string reason_;
  engine::ui::Canvas canvas_;
  std::shared_ptr<engine::ui::TextElement> title_;
  std::shared_ptr<engine::ui::TextElement> reason_text_;
  std::shared_ptr<engine::ui::TextElement> action_text_;
};

}  // namespace client

#endif  // CLIENT_SCENE_DISCONNECTED_SCENE_H_
