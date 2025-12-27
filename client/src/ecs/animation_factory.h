/**
 * @file animation_factory.h
 * @brief Animation component factory for client archetypes
 *
 * @details
 * Creates frame lists for entities based on their archetype kind
 */

#ifndef CLIENT_ECS_ANIMATION_FACTORY_H_
#define CLIENT_ECS_ANIMATION_FACTORY_H_

#include <cstdint>

#include "ecs/archetype_registry.h"
#include "engine/ecs/registry.h"

namespace client::ecs {

/**
 * @class AnimationFactory
 * @brief Factory for assigning animation components to entities
 */
class AnimationFactory {
 public:
  /**
   * @brief Create a factory backed by the archetype registry
   * @param archetypes Archetype registry reference
   */
  explicit AnimationFactory(const ArchetypeRegistry& archetypes);

  /**
   * @brief Ensure an entity has animation frames for its archetype
   * @param registry ECS registry
   * @param entity Entity to update
   * @param type_code Type code from the snapshot
   */
  void EnsureAnimation(engine::ecs::Registry& registry,
                       engine::ecs::EntityId entity,
                       std::uint16_t type_code) const;

 private:
  void ApplyAnimation(engine::ecs::Registry& registry,
                      engine::ecs::EntityId entity,
                      ArchetypeKind kind) const;

  const ArchetypeRegistry& archetypes_;
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_ANIMATION_FACTORY_H_
