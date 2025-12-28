/**
 * @file sprite_sync_system.h
 * @brief Sync ECS sprite components from archetype data
 */

#ifndef CLIENT_ECS_SPRITE_SYNC_SYSTEM_H_
#define CLIENT_ECS_SPRITE_SYNC_SYSTEM_H_

#include "ecs/archetype_registry.h"
#include "engine/ecs/registry.h"

namespace client::ecs {

/**
 * @class SpriteSyncSystem
 * @brief Updates sprite and layer components from archetype registry
 */
class SpriteSyncSystem {
 public:
  /**
   * @brief Create a sprite sync system
   * @param registry ECS registry reference
   */
  explicit SpriteSyncSystem(engine::ecs::Registry& registry);

  /**
   * @brief Populate sprite components from networked entities
   */
  void SyncSprites();

 private:
  void RegisterComponents();
  void ApplyDefinition(std::size_t index, const SpriteDefinition& definition);

  engine::ecs::Registry& registry_;
  const ArchetypeRegistry& archetypes_;
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_SPRITE_SYNC_SYSTEM_H_
