#ifndef ENGINE_ECS_COMPONENTS_TAG_COMPONENT_H_
#define ENGINE_ECS_COMPONENTS_TAG_COMPONENT_H_

#include <string>

namespace engine::ecs {

/**
 * @brief String-based entity identification/grouping
 *
 * @details
 * Allows tagging entities with strings for filtering and queries.
 * Common tags: "Player", "Enemy", "Projectile", "Obstacle", etc.
 */
struct TagComponent {
  std::string tag;

  TagComponent() = default;
  explicit TagComponent(std::string t) : tag(std::move(t)) {}
};

}  // namespace engine::ecs

#endif  // ENGINE_ECS_COMPONENTS_TAG_COMPONENT_H_
