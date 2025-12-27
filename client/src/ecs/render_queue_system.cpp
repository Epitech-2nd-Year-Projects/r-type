#include "ecs/render_queue_system.h"

#include <algorithm>
#include <cmath>

namespace client::ecs {

namespace {

constexpr std::int32_t kBackgroundLayerMax = 2;
constexpr std::int32_t kForegroundLayerMin = 9;

engine::render::RenderLayer ResolveRenderLayer(std::int32_t value) {
  if (value <= kBackgroundLayerMax) {
    return engine::render::RenderLayer::kBackground;
  }
  if (value >= kForegroundLayerMin) {
    return engine::render::RenderLayer::kForeground;
  }
  return engine::render::RenderLayer::kMidground;
}

}  // namespace

RenderQueueSystem::RenderQueueSystem(engine::ecs::Registry& registry,
                                     engine::render::Renderer2D& renderer)
    : registry_(registry),
      renderer_(renderer),
      archetypes_(ArchetypeRegistry::Get()) {
  RegisterComponents();
}

void RenderQueueSystem::RegisterComponents() {
  registry_.RegisterComponent<ecs::SpriteComponent>();
  registry_.RegisterComponent<ecs::RenderLayerComponent>();
  registry_.RegisterComponent<ecs::PositionComponent>();
  registry_.RegisterComponent<ecs::NetworkedEntityComponent>();
  registry_.RegisterComponent<ecs::VelocityComponent>();
}

void RenderQueueSystem::Reset() {
  textures_.clear();
  draw_queue_.clear();
}

void RenderQueueSystem::Render() {
  draw_queue_.clear();

  const auto& positions = registry_.GetComponents<ecs::PositionComponent>();
  const auto& sprites = registry_.GetComponents<ecs::SpriteComponent>();
  const auto& layers = registry_.GetComponents<ecs::RenderLayerComponent>();
  const auto& nets = registry_.GetComponents<ecs::NetworkedEntityComponent>();
  const auto& velocities = registry_.GetComponents<ecs::VelocityComponent>();

  const std::size_t count = positions.size();

  for (std::size_t i = 0; i < count; ++i) {
    if (!positions[i].has_value()) {
      continue;
    }
    if (i >= sprites.size() || !sprites[i].has_value()) {
      continue;
    }
    if (i >= layers.size() || !layers[i].has_value()) {
      continue;
    }
    if (i >= nets.size() || !nets[i].has_value()) {
      continue;
    }
    if (!sprites[i]->visible) {
      continue;
    }

    const auto texture = LoadTexture(sprites[i]->texture_id);
    if (!texture) {
      continue;
    }

    const auto velocity = (i < velocities.size())
                              ? velocities[i]
                              : std::optional<ecs::VelocityComponent>{};

    const auto params = BuildParams(positions[i].value(), sprites[i].value(),
                                    layers[i].value(), velocity,
                                    nets[i]->type_code, texture);

    draw_queue_.push_back(
        DrawCommand{texture, params, layers[i]->layer, layers[i]->depth, i});
  }

  std::sort(draw_queue_.begin(), draw_queue_.end(),
            [](const DrawCommand& a, const DrawCommand& b) {
              if (a.layer != b.layer) return a.layer < b.layer;
              if (a.depth != b.depth) return a.depth < b.depth;
              return a.entity_index < b.entity_index;
            });

  for (const auto& cmd : draw_queue_) {
    renderer_.DrawTexture(*cmd.texture, cmd.params);
  }
}

engine::render::SpriteDrawParams RenderQueueSystem::BuildParams(
    const ecs::PositionComponent& position, const ecs::SpriteComponent& sprite,
    const ecs::RenderLayerComponent& layer,
    const std::optional<ecs::VelocityComponent>& velocity,
    std::uint16_t type_code,
    const std::shared_ptr<engine::render::Texture2D>& texture) const {
  engine::render::SpriteDrawParams params{};
  params.position = position.render_position;
  params.layer = ResolveRenderLayer(layer.layer);

  const bool flip_x = ComputeFlipX(type_code, velocity, sprite.flip_x);
  const bool flip_y = sprite.flip_y;

  const auto source = ApplyFlip(sprite.source_rect, flip_x, flip_y);
  params.source = source;
  params.scale = ComputeScale(*texture, source);
  params.rotation = 0.0f;
  params.tint = sprite.tint;
  return params;
}

engine::math::RectF RenderQueueSystem::ApplyFlip(
    const engine::math::RectF& base, bool flip_x, bool flip_y) const {
  engine::math::RectF rect = base;
  if (flip_x) {
    rect.top_left_x_ = base.top_left_x_ + base.width_;
    rect.width_ = -base.width_;
  }
  if (flip_y) {
    rect.top_left_y_ = base.top_left_y_ + base.height_;
    rect.height_ = -base.height_;
  }
  return rect;
}

engine::math::Vector2f RenderQueueSystem::ComputeScale(
    const engine::render::Texture2D& texture,
    const engine::math::RectF& source) const {
  const engine::math::Vector2i size = texture.GetSize();
  const float width = std::abs(source.width_);
  const float height = std::abs(source.height_);
  if (size.x == 0 || size.y == 0) {
    return {1.0f, 1.0f};
  }
  return {width / static_cast<float>(size.x),
          height / static_cast<float>(size.y)};
}

bool RenderQueueSystem::ComputeFlipX(
    std::uint16_t type_code,
    const std::optional<ecs::VelocityComponent>& velocity,
    bool sprite_flip) const {
  bool flip = sprite_flip;
  if (velocity.has_value()) {
    const float vx = velocity->velocity.x;
    if (std::abs(vx) > 1.0f) {
      flip = (vx < 0.0f) != sprite_flip;
    }
  }

  if (!velocity.has_value() && archetypes_.IsEnemy(type_code)) {
    flip = !sprite_flip;
  }

  return flip;
}

std::shared_ptr<engine::render::Texture2D> RenderQueueSystem::LoadTexture(
    const std::string& id) {
  if (id.empty()) {
    return nullptr;
  }

  const auto it = textures_.find(id);
  if (it != textures_.end()) {
    return it->second;
  }

  try {
    auto texture = renderer_.LoadTextureFromFile(id);
    textures_.emplace(id, texture);
    return texture;
  } catch (...) {
    return nullptr;
  }
}

}  // namespace client::ecs
