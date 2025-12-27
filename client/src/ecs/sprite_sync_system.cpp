#include "ecs/sprite_sync_system.h"

namespace client::ecs {

SpriteSyncSystem::SpriteSyncSystem(engine::ecs::Registry& registry)
    : registry_(registry), archetypes_(ArchetypeRegistry::Get()) {
  RegisterComponents();
}

void SpriteSyncSystem::RegisterComponents() {
  registry_.RegisterComponent<ecs::SpriteComponent>();
  registry_.RegisterComponent<ecs::RenderLayerComponent>();
  registry_.RegisterComponent<ecs::PositionComponent>();
  registry_.RegisterComponent<ecs::NetworkedEntityComponent>();
  registry_.RegisterComponent<ecs::VelocityComponent>();
  registry_.RegisterComponent<ecs::HealthComponent>();
}

void SpriteSyncSystem::SyncSprites() {
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

    SpriteContext context{};
    context.network_id = nets[i]->network_id;
    context.health = health;
    context.velocity = velocity;
    context.entity_index = i;

    const auto definition =
        archetypes_.ResolveSprite(nets[i]->type_code, context);
    if (!definition.has_value()) {
      continue;
    }

    ApplyDefinition(i, *definition);
  }
}

void SpriteSyncSystem::ApplyDefinition(std::size_t index,
                                       const SpriteDefinition& definition) {
  auto& sprites = registry_.GetComponents<ecs::SpriteComponent>();
  auto& layers = registry_.GetComponents<ecs::RenderLayerComponent>();

  auto& sprite = sprites[index];
  if (!sprite.has_value()) {
    sprite =
        ecs::SpriteComponent(definition.texture_id, definition.source_rect);
    sprite->flip_x = definition.face_left;
    sprite->tint = definition.tint;
  } else {
    if (sprite->texture_id.empty()) {
      sprite->texture_id = definition.texture_id;
    }
    if (sprite->source_rect.width_ == 0.0f ||
        sprite->source_rect.height_ == 0.0f) {
      sprite->source_rect = definition.source_rect;
    }
    sprite->flip_x = definition.face_left;
    sprite->tint = definition.tint;
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

}  // namespace client::ecs
