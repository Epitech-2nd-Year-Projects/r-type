#ifndef RIFT_CLIENT_APPLICATION_H_
#define RIFT_CLIENT_APPLICATION_H_

#include <memory>

#include "engine/time/time_delta.h"
#include "input/fight_input.h"
#include "input/input_sender.h"
#include "network_session.h"
#include "rift_config.h"
#include "rift_context.h"

namespace engine::app {
class EngineRuntime;
}

namespace engine::ecs {
class Registry;
}

namespace rift::client {

class FightScene;

class Application : public RiftContext {
 public:
  explicit Application(RiftConfig config);
  ~Application() override;

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  int Run();

  engine::render::Renderer2D& Renderer() override;
  engine::render::Renderer3D& Renderer3D() override;
  engine::input::InputManager& Input() override;
  engine::render::Window& Window() override;
  engine::math::Vector2i RenderSize() const override;

  engine::ecs::Registry& World() override;
  const engine::ecs::Registry& World() const override;

  bool EnqueueCommand(const protocol::CommandPayload& payload) override;
  std::optional<std::uint32_t> LocalPlayerId() const override;
  std::optional<float> LatestLatencyMs() const override;

  RiftClientState State() const override;
  bool IsConnected() const override;

  FightActionState GetInputState() const override;

 private:
  bool Initialize();
  bool Tick(engine::time::TimeDelta dt);
  void HandleNetworkEvents(const NetworkEvents& events);
  void UpdateGameState();
  void Render();

  RiftConfig config_;
  std::unique_ptr<engine::app::EngineRuntime> engine_;
  std::unique_ptr<NetworkSession> network_;
  std::unique_ptr<FightInputLayer> input_layer_;
  std::unique_ptr<InputSender> input_sender_;
  std::unique_ptr<FightScene> scene_;
  RiftClientState state_{RiftClientState::kConnecting};
  engine::math::Vector2i render_size_{1280, 720};
  bool ready_sent_{false};
  bool should_quit_{false};
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_APPLICATION_H_
