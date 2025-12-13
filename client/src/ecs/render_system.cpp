#include "ecs/render_system.h"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

#include "engine/math/vector2.h"
#include "game_logic/constants.h"
#include "game_logic/entities/enemy_data.h"
#include "game_logic/entities/missile_data.h"
#include "game_logic/entities/obstacle_data.h"

namespace client::ecs {

namespace {

using SpriteDefinition = RenderSystem::SpriteDefinition;

constexpr std::uint16_t kPlayerTypeCode = 1u;
constexpr std::uint16_t kEnemyTypeCode = 2u;
constexpr std::uint16_t kMissileTypeCode = 3u;
constexpr std::uint16_t kObstacleTypeCode = 4u;

SpriteDefinition MakeDefinition(std::string_view texture, float width,
                                float height, std::int32_t layer, float depth,
                                bool face_left) {
  SpriteDefinition def{};
  def.texture_id = texture;
  def.source_rect = engine::math::RectF(0.0f, 0.0f, width, height);
  def.layer = layer;
  def.depth = depth;
  def.face_left = face_left;
  return def;
}

SpriteDefinition EnemyDefinition(
    const game_logic::entities::EnemyArchetypeData& data, float depth) {
  return MakeDefinition(data.texture_path, data.sprite_width,
                        data.sprite_height, game_logic::kEnemyLayer, depth,
                        true);
}

SpriteDefinition MissileDefinition(
    const game_logic::entities::MissileArchetypeData& data, bool face_left) {
  return MakeDefinition(data.texture_path, data.sprite_width,
                        data.sprite_height, 8, 0.0f, face_left);
}

SpriteDefinition ObstacleDefinition(
    const game_logic::entities::ObstacleArchetypeData& data) {
  return MakeDefinition(data.texture_path, 64.0f, 64.0f, 3, 0.0f, false);
}

SpriteDefinition DefaultPlayer() {
  return MakeDefinition("assets/sprites/player.png", 32.0f, 16.0f, 10, 0.0f,
                        false);
}

engine::render::RenderLayer ResolveRenderLayer(std::int32_t value) {
  if (value <= 2) {
    return engine::render::RenderLayer::kBackground;
  }
  if (value >= 9) {
    return engine::render::RenderLayer::kForeground;
  }
  return engine::render::RenderLayer::kMidground;
}

}  // namespace

RenderSystem::RenderSystem(engine::ecs::Registry& registry,
                           engine::render::Renderer2D& renderer)
    : registry_(registry), renderer_(renderer) {
  RegisterComponents();
}

void RenderSystem::RegisterComponents() {
  registry_.RegisterComponent<ecs::SpriteComponent>();
  registry_.RegisterComponent<ecs::RenderLayerComponent>();
  registry_.RegisterComponent<ecs::PositionComponent>();
  registry_.RegisterComponent<ecs::NetworkedEntityComponent>();
  registry_.RegisterComponent<ecs::VelocityComponent>();
  registry_.RegisterComponent<ecs::HealthComponent>();
}

void RenderSystem::Reset() {
  textures_.clear();
  draw_queue_.clear();
}

void RenderSystem::Render() {
  draw_queue_.clear();

  auto& sprites = registry_.GetComponents<ecs::SpriteComponent>();
  auto& layers = registry_.GetComponents<ecs::RenderLayerComponent>();
  const auto& positions = registry_.GetComponents<ecs::PositionComponent>();
  const auto& nets = registry_.GetComponents<ecs::NetworkedEntityComponent>();
  const auto& velocities = registry_.GetComponents<ecs::VelocityComponent>();
  const auto& healths = registry_.GetComponents<ecs::HealthComponent>();

  const std::size_t count = positions.size();

  for (std::size_t i = 0; i < count; ++i) {
    if (!positions[i].has_value()) {
      continue;
    }
    if (i >= nets.size() || !nets[i].has_value()) {
      continue;
    }

    const auto health = (i < healths.size())
                            ? healths[i]
                            : std::optional<ecs::HealthComponent>{};
    const auto velocity = (i < velocities.size())
                              ? velocities[i]
                              : std::optional<ecs::VelocityComponent>{};

    auto definition = ResolveDefinition(*nets[i], health, velocity, i);
    if (!definition.has_value()) {
      continue;
    }

    SyncSprite(i, *definition, health, velocity);

    if (!sprites[i].has_value() || !layers[i].has_value() ||
        !sprites[i]->visible) {
      continue;
    }

    const auto texture = LoadTexture(sprites[i]->texture_id);
    if (!texture) {
      continue;
    }

    const auto params = BuildParams(
        positions[i].value(), sprites[i].value(), layers[i].value(), velocity,
        nets[i]->type_code, definition->face_left, texture);

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

void RenderSystem::SyncSprite(
    std::size_t index, const SpriteDefinition& definition,
    const std::optional<ecs::HealthComponent>& /*health*/,
    const std::optional<ecs::VelocityComponent>& /*velocity*/) {
  auto& sprites = registry_.GetComponents<ecs::SpriteComponent>();
  auto& layers = registry_.GetComponents<ecs::RenderLayerComponent>();

  auto& sprite = sprites[index];
  if (!sprite.has_value()) {
    sprite =
        ecs::SpriteComponent(definition.texture_id, definition.source_rect);
    sprite->flip_x = definition.face_left;
  } else {
    if (sprite->texture_id.empty()) {
      sprite->texture_id = definition.texture_id;
    }
    if (sprite->source_rect.width_ == 0.0f ||
        sprite->source_rect.height_ == 0.0f) {
      sprite->source_rect = definition.source_rect;
    }
    if (!sprite->flip_x && definition.face_left) {
      sprite->flip_x = true;
    }
  }
  sprite->visible = true;

  auto& layer = layers[index];
  if (!layer.has_value()) {
    layer = ecs::RenderLayerComponent(definition.layer, definition.depth);
  } else {
    layer->layer = definition.layer;
    if (layer->depth == 0.0f) {
      layer->depth = definition.depth;
    }
  }
}

std::optional<RenderSystem::SpriteDefinition> RenderSystem::ResolveDefinition(
    const ecs::NetworkedEntityComponent& net,
    const std::optional<ecs::HealthComponent>& health,
    const std::optional<ecs::VelocityComponent>& velocity,
    std::size_t entity_index) const {
  switch (net.type_code) {
    case kPlayerTypeCode:
      return DefaultPlayer();
    case kEnemyTypeCode:
      return ResolveEnemy(health, velocity, entity_index);
    case kMissileTypeCode:
      return ResolveMissile(velocity);
    case kObstacleTypeCode:
      return ResolveObstacle(health);
    default:
      return std::nullopt;
  }
}

RenderSystem::SpriteDefinition RenderSystem::ResolveEnemy(
    const std::optional<ecs::HealthComponent>& health,
    const std::optional<ecs::VelocityComponent>& velocity,
    std::size_t entity_index) const {
  const auto hp =
      health.has_value() ? static_cast<std::uint32_t>(health->max) : 0u;
  const float speed = velocity.has_value() ? velocity->velocity.Length() : 0.0f;

  if (hp >= game_logic::entities::kTankData.health) {
    return EnemyDefinition(game_logic::entities::kTankData, 0.3f);
  }
  if (hp >= game_logic::entities::kBomberData.health) {
    return EnemyDefinition(game_logic::entities::kBomberData, 0.2f);
  }
  if (speed > 170.0f ||
      (velocity.has_value() && std::abs(velocity->velocity.y) > 15.0f)) {
    return EnemyDefinition(game_logic::entities::kInterceptorData, 0.1f);
  }

  if (entity_index % 2 == 1) {
    return EnemyDefinition(game_logic::entities::kInterceptorData, 0.1f);
  }
  return EnemyDefinition(game_logic::entities::kScoutData, 0.0f);
}

RenderSystem::SpriteDefinition RenderSystem::ResolveMissile(
    const std::optional<ecs::VelocityComponent>& velocity) const {
  const float vx = velocity.has_value() ? velocity->velocity.x : 1.0f;
  const float speed = velocity.has_value() ? velocity->velocity.Length() : 0.0f;

  if (vx < -5.0f) {
    return MissileDefinition(game_logic::entities::kEnemyMissileData, true);
  }
  if (speed > 260.0f) {
    return MissileDefinition(game_logic::entities::kPlayerMissileData, false);
  }
  if (speed > 0.0f && speed < 220.0f) {
    return MissileDefinition(game_logic::entities::kNeutralMissileData,
                             vx < 0.0f);
  }
  return MissileDefinition(game_logic::entities::kBigPlayerMissileData, false);
}

RenderSystem::SpriteDefinition RenderSystem::ResolveObstacle(
    const std::optional<ecs::HealthComponent>& health) const {
  if (health.has_value() && health->max == 0u) {
    return ObstacleDefinition(game_logic::entities::kWallData);
  }
  return ObstacleDefinition(game_logic::entities::kDestructibleBarrierData);
}

engine::render::SpriteDrawParams RenderSystem::BuildParams(
    const ecs::PositionComponent& position, const ecs::SpriteComponent& sprite,
    const ecs::RenderLayerComponent& layer,
    const std::optional<ecs::VelocityComponent>& velocity,
    std::uint16_t type_code, bool default_face_left,
    const std::shared_ptr<engine::render::Texture2D>& texture) const {
  engine::render::SpriteDrawParams params{};
  params.position = position.render_position;
  params.layer = ResolveRenderLayer(layer.layer);

  const bool flip_x =
      ComputeFlipX(type_code, default_face_left, velocity, sprite.flip_x);
  const bool flip_y = sprite.flip_y;

  const auto source = ApplyFlip(sprite.source_rect, flip_x, flip_y);
  params.source = source;
  params.scale = ComputeScale(*texture, source);
  params.rotation = 0.0f;
  params.tint = engine::render::Color::White();
  return params;
}

engine::math::RectF RenderSystem::ApplyFlip(const engine::math::RectF& base,
                                            bool flip_x, bool flip_y) const {
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

engine::math::Vector2f RenderSystem::ComputeScale(
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

bool RenderSystem::ComputeFlipX(
    std::uint16_t type_code, bool default_left,
    const std::optional<ecs::VelocityComponent>& velocity,
    bool sprite_flip) const {
  bool flip = sprite_flip || default_left;
  if (velocity.has_value()) {
    const float vx = velocity->velocity.x;
    if (std::abs(vx) > 1.0f) {
      flip = vx < 0.0f;
    }
  }

  if (type_code == kEnemyTypeCode && !velocity.has_value()) {
    flip = true;
  }

  return flip;
}

std::shared_ptr<engine::render::Texture2D> RenderSystem::LoadTexture(
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
