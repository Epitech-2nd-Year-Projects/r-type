#include "ecs/render_system.h"

namespace client::ecs {

RenderSystem::RenderSystem(engine::ecs::Registry& registry,
                           engine::render::Renderer2D& renderer)
    : sprite_sync_(registry), render_queue_(registry, renderer) {}

void RenderSystem::Reset() { render_queue_.Reset(); }

void RenderSystem::Render() {
  sprite_sync_.SyncSprites();
  render_queue_.Render();
}

}  // namespace client::ecs
