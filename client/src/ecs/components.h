/**
 * @file components.h
 * @brief Client-side ECS components for rendering and world state
 *
 * @details
 * Defines lightweight components describing networked entities and local-only
 * tags used by the client. These components track spatial state for
 * interpolation, rendering metadata, basic health, and classification tags
 * used for gameplay presentation.
 */

#ifndef CLIENT_ECS_COMPONENTS_H_
#define CLIENT_ECS_COMPONENTS_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "engine/math/rect.h"
#include "engine/math/vector2.h"

namespace client::ecs {

/**
 * @brief Stable identity for entities synchronized from the server
 *
 * @details
 * Stores the authoritative network identifier, archetype/type code, and the
 * most recent snapshot identifier that updated this entity. Consumers can use
 * this to match incoming deltas to existing entities and avoid applying stale
 * updates.
 */
struct NetworkedEntityComponent {
  std::uint32_t network_id{0};     ///< EntityId from the authoritative server.
  std::uint16_t type_code{0};      ///< Archetype or type classification.
  std::uint32_t last_snapshot{0};  ///< Snapshot identifier of the last update.
  std::uint8_t flags{0};           ///< Status flags (e.g. ready state).

  NetworkedEntityComponent() = default;
  NetworkedEntityComponent(std::uint32_t id, std::uint16_t type,
                           std::uint32_t snapshot, std::uint8_t f = 0)
      : network_id(id), type_code(type), last_snapshot(snapshot), flags(f) {}
};

/**
 * @brief World-space position for rendering and interpolation
 *
 * @details
 * Tracks the latest authoritative position and the previous position so that
 * interpolation systems can smoothly transition between snapshots. The
 * render_position field stores the blended position used for drawing and may
 * differ slightly from the authoritative state while smoothing corrections.
 */
struct PositionComponent {
  engine::math::Vector2f position{0.0f, 0.0f};
  engine::math::Vector2f previous_position{0.0f, 0.0f};
  engine::math::Vector2f render_position{0.0f, 0.0f};

  PositionComponent() = default;
  PositionComponent(float x, float y)
      : position(x, y), previous_position(x, y), render_position(x, y) {}
  PositionComponent(const engine::math::Vector2f& pos,
                    const engine::math::Vector2f& prev)
      : position(pos), previous_position(prev), render_position(pos) {}
};

/**
 * @brief Linear velocity used for interpolation and prediction
 *
 * @details
 * Represents the latest velocity received from the server. Rendering or
 * simulation systems can use this to extrapolate positions between snapshots.
 */
struct VelocityComponent {
  engine::math::Vector2f velocity{0.0f, 0.0f};

  VelocityComponent() = default;
  VelocityComponent(float vx, float vy) : velocity(vx, vy) {}
  explicit VelocityComponent(const engine::math::Vector2f& vel)
      : velocity(vel) {}
};

/**
 * @brief Sprite description for rendering
 *
 * @details
 * Holds the sprite asset identifier, source rectangle, visibility, and flip
 * states required by the client renderer.
 */
struct SpriteComponent {
  std::string texture_id;  ///< Texture asset identifier or path.
  engine::math::RectF source_rect{0.0f, 0.0f, 32.0f,
                                  32.0f};  ///< Source rectangle.
  bool visible{true};                      ///< Whether to draw the sprite.
  bool flip_x{false};                      ///< Flip horizontally.
  bool flip_y{false};                      ///< Flip vertically.

  SpriteComponent() = default;
  explicit SpriteComponent(std::string id) : texture_id(std::move(id)) {}
  SpriteComponent(std::string id, const engine::math::RectF& rect)
      : texture_id(std::move(id)), source_rect(rect) {}
};

/**
 * @brief Frame-based sprite animation
 */
struct AnimationComponent {
  std::vector<engine::math::RectF> frames;
  float frame_duration{0.1f};
  float timer{0.0f};
  std::size_t current_frame{0};
  bool loop{true};
  bool playing{true};

  AnimationComponent() = default;
  AnimationComponent(std::vector<engine::math::RectF> f, float duration)
      : frames(std::move(f)), frame_duration(duration) {}
};

/**
 * @brief Render ordering metadata
 *
 * @details
 * Provides layer and depth values to sort draw calls. Higher layer is drawn on
 * top, depth is a secondary tie breaker within a layer.
 */
struct RenderLayerComponent {
  std::int32_t layer{0};
  float depth{0.0f};

  RenderLayerComponent() = default;
  RenderLayerComponent(std::int32_t l, float d) : layer(l), depth(d) {}
};

/**
 * @brief Client-side health representation
 *
 * @details
 * Stores health values sent by the server for UI or effects. Values are kept
 * small because network payloads quantize health to a byte.
 */
struct HealthComponent {
  std::uint8_t current{0};
  std::uint8_t max{0};

  HealthComponent() = default;
  explicit HealthComponent(std::uint8_t hp) : current(hp), max(hp) {}
  HealthComponent(std::uint8_t current_hp, std::uint8_t max_hp)
      : current(current_hp), max(max_hp) {}

  /**
   * @brief Helper to check if the entity is alive
   * @return true when health is above zero
   */
  bool alive() const { return current > 0; }
};

/**
 * @brief Player statistics received from the server
 */
struct PlayerStateComponent {
  std::uint32_t player_id{0};
  std::uint32_t score{0};
  std::uint8_t lives{0};

  PlayerStateComponent() = default;
  PlayerStateComponent(std::uint32_t id, std::uint32_t s, std::uint8_t l)
      : player_id(id), score(s), lives(l) {}
};

/**
 * @brief Tag marking a player entity from the server
 */
struct PlayerTag {};

/**
 * @brief Tag marking an enemy entity from the server
 */
struct EnemyTag {};

/**
 * @brief Tag marking a missile or projectile entity
 */
struct MissileTag {};

/**
 * @brief Tag for the locally controlled player entity
 */
struct LocalPlayerTag {};

/**
 * @brief Snapshot timing metadata for interpolation
 *
 * @details
 * Stores the timestamps of the last two snapshots applied to an entity so
 * interpolation systems can compute blend factors and gracefully fall back to
 * extrapolation when new snapshots have not arrived yet.
 */
struct SnapshotInterpolationComponent {
  std::uint64_t previous_snapshot_ms{0};
  std::uint64_t last_snapshot_ms{0};

  SnapshotInterpolationComponent() = default;
  SnapshotInterpolationComponent(std::uint64_t last_snapshot_ms,
                                 std::uint64_t previous_snapshot_ms)
      : previous_snapshot_ms(previous_snapshot_ms),
        last_snapshot_ms(last_snapshot_ms) {}
};

}  // namespace client::ecs

#endif  // CLIENT_ECS_COMPONENTS_H_
