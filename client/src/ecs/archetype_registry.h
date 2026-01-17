/**
 * @file archetype_registry.h
 * @brief Client archetype registry for ECS classification and visuals
 *
 * @details
 * Provides shared lookup for type codes and sprite definitions
 * Used by render audio hud and prediction systems
 */

#ifndef CLIENT_ECS_ARCHETYPE_REGISTRY_H_
#define CLIENT_ECS_ARCHETYPE_REGISTRY_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#include "ecs/components.h"

namespace client::ecs {

/**
 * @enum ArchetypeKind
 * @brief High level archetype grouping for networked entities
 */
enum class ArchetypeKind {
  kUnknown = 0,
  kPlayer,
  kEnemy,
  kMissile,
  kObstacle,
  kPowerup
};

/**
 * @struct SpriteDefinition
 * @brief Sprite description resolved from an archetype entry
 */
struct SpriteDefinition {
  std::string texture_id;
  engine::math::RectF source_rect;
  std::int32_t layer{0};
  float depth{0.0f};
  bool face_left{false};
  engine::render::Color tint{engine::render::Color::White()};
  std::optional<engine::math::Vector2f> render_size;
  bool use_full_source{false};
};

/**
 * @struct SpriteContext
 * @brief Context data used to resolve sprite selections
 */
struct SpriteContext {
  std::uint32_t network_id{0};
  std::optional<HealthComponent> health;
  std::optional<VelocityComponent> velocity;
  std::size_t entity_index{0};
};

/**
 * @struct ArchetypeDefinition
 * @brief Shared archetype metadata for client systems
 */
struct ArchetypeDefinition {
  std::uint16_t type_code{0};
  ArchetypeKind kind{ArchetypeKind::kUnknown};
  bool damageable{false};
  bool explosive{false};
};

/**
 * @class ArchetypeRegistry
 * @brief Central registry for archetype classification and sprites
 */
class ArchetypeRegistry {
 public:
  /**
   * @brief Access the shared registry instance
   * @return Registry reference
   */
  static const ArchetypeRegistry& Get();

  /**
   * @brief Lookup metadata for a type code
   * @param type_code Type code from the network snapshot
   * @return Definition pointer or null when unknown
   */
  std::optional<std::reference_wrapper<const ArchetypeDefinition>> Find(
      std::uint16_t type_code) const;

  /**
   * @brief Resolve the archetype kind for a type code
   * @param type_code Type code from the network snapshot
   * @return Archetype kind
   */
  ArchetypeKind KindOf(std::uint16_t type_code) const;

  /**
   * @brief Check if the type code matches a specific kind
   * @param type_code Type code from the network snapshot
   * @param kind Target archetype kind
   * @return True when the kind matches
   */
  bool IsKind(std::uint16_t type_code, ArchetypeKind kind) const;

  /**
   * @brief Check if a type code represents a player
   * @param type_code Type code from the network snapshot
   * @return True when the archetype is a player
   */
  bool IsPlayer(std::uint16_t type_code) const;

  /**
   * @brief Check if a type code represents an enemy
   * @param type_code Type code from the network snapshot
   * @return True when the archetype is an enemy
   */
  bool IsEnemy(std::uint16_t type_code) const;

  /**
   * @brief Check if a type code represents a missile
   * @param type_code Type code from the network snapshot
   * @return True when the archetype is a missile
   */
  bool IsMissile(std::uint16_t type_code) const;

  /**
   * @brief Check if a type code represents an obstacle
   * @param type_code Type code from the network snapshot
   * @return True when the archetype is an obstacle
   */
  bool IsObstacle(std::uint16_t type_code) const;

  /**
   * @brief Check if a type code represents a powerup
   * @param type_code Type code from the network snapshot
   * @return True when the archetype is a powerup
   */
  bool IsPowerup(std::uint16_t type_code) const;

  /**
   * @brief Check if the archetype is damageable
   * @param type_code Type code from the network snapshot
   * @return True when the archetype is damageable
   */
  bool IsDamageable(std::uint16_t type_code) const;

  /**
   * @brief Check if the archetype should trigger explosions
   * @param type_code Type code from the network snapshot
   * @return True when the archetype is explosive
   */
  bool IsExplosive(std::uint16_t type_code) const;

  /**
   * @brief Resolve sprite definition for the given archetype
   * @param type_code Type code from the network snapshot
   * @param context State used to pick sprite variants
   * @return Sprite definition when available
   */
  std::optional<SpriteDefinition> ResolveSprite(
      std::uint16_t type_code, const SpriteContext& context) const;

  /**
   * @brief Register a dynamic powerup type from configuration
   * @param type_code Network type code (10 + PowerupType)
   * @param texture_path Path to the sprite texture
   */
  static void RegisterPowerupType(std::uint16_t type_code,
                                  std::string_view texture_path, float width,
                                  float height);

  /**
   * @brief Register a dynamic enemy type from configuration
   * @param type_code Network type code
   * @param texture_path Path to the sprite texture
   * @param width Render width associated with the type
   * @param height Render height associated with the type
   */
  static void RegisterEnemyType(std::uint16_t type_code,
                                std::string_view texture_path, float width,
                                float height, float frame_width,
                                float frame_height);

  /**
   * @brief Configure player sprite dimensions
   * @param width Render width
   * @param height Render height
   * @param frame_width Source frame width
   * @param frame_height Source frame height
   */
  static void SetPlayerConfig(float width, float height, float frame_width,
                              float frame_height);

 private:
  ArchetypeRegistry();

  static ArchetypeRegistry& Mutable();

  std::unordered_map<std::uint16_t, SpriteDefinition> custom_powerups_;
  std::unordered_map<std::uint16_t, SpriteDefinition> custom_enemies_;
  std::unordered_map<std::uint16_t, ArchetypeDefinition> custom_definitions_;

  engine::math::Vector2f player_render_size_{33.0f, 17.0f};
  engine::math::Vector2f player_frame_size_{33.0f, 17.0f};
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_ARCHETYPE_REGISTRY_H_
