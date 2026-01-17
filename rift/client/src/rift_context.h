#ifndef RIFT_CLIENT_RIFT_CONTEXT_H_
#define RIFT_CLIENT_RIFT_CONTEXT_H_

#include <cstdint>
#include <optional>

#include "engine/math/vector2.h"
#include "input/fight_input.h"
#include "protocol/command.h"
#include "rift_state.h"

namespace engine::ecs {
class Registry;
}  // namespace engine::ecs

namespace engine::input {
class InputManager;
}  // namespace engine::input

namespace engine::render {
class Renderer2D;
class Renderer3D;
class Window;
}  // namespace engine::render

namespace rift::client {

class RiftContext {
 public:
  virtual ~RiftContext() = default;

  virtual engine::render::Renderer2D& Renderer() = 0;
  virtual engine::render::Renderer3D& Renderer3D() = 0;
  virtual engine::input::InputManager& Input() = 0;
  virtual engine::render::Window& Window() = 0;
  virtual engine::math::Vector2i RenderSize() const = 0;

  virtual engine::ecs::Registry& World() = 0;
  virtual const engine::ecs::Registry& World() const = 0;

  virtual bool EnqueueCommand(const protocol::CommandPayload& payload) = 0;
  virtual std::optional<std::uint32_t> LocalPlayerId() const = 0;
  virtual std::optional<float> LatestLatencyMs() const = 0;

  virtual RiftClientState State() const = 0;
  virtual bool IsConnected() const = 0;

  virtual FightActionState GetInputState() const = 0;

  virtual std::uint32_t RoundTimerMs() const = 0;
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_RIFT_CONTEXT_H_
