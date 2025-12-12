/**
 * @file sound_effects.h
 * @brief Client-side sound effect routing for gameplay feedback
 */

#ifndef CLIENT_SOUND_EFFECTS_H_
#define CLIENT_SOUND_EFFECTS_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include "engine/audio/audio_engine.h"
#include "engine/ecs/registry.h"

namespace client {

/**
 * @class SoundEffects
 * @brief Triggers sound effects from gameplay events and entity state changes
 *
 * Observes snapshots applied to the client ECS world and plays short audio
 * cues for shots, explosions, damage, and player death notifications.
 */
class SoundEffects {
 public:
  /**
   * @brief Create a sound effect router backed by the engine audio subsystem
   */
  explicit SoundEffects(engine::audio::AudioEngine& engine);

  /**
   * @brief Preload configured sound effect assets
   */
  void LoadAssets();

  /**
   * @brief Clear cached entity state after disconnects or resets
   */
  void Reset();

  /**
   * @brief Inspect the current ECS state and emit audio cues for changes
   *
   * @param registry Client-side ECS registry after the latest snapshot
   */
  void OnSnapshotApplied(const engine::ecs::Registry& registry);

  /**
   * @brief Emit audio feedback for a player death event
   */
  void OnPlayerDeath();

 private:
  enum class EffectType { kShot, kExplosion, kDamage, kPlayerDeath };

  struct EntityAudioState {
    std::uint16_t type_code{0};
    std::uint8_t health{0};
    bool has_health{false};
    bool alive{true};
  };

  struct EffectHasher {
    std::size_t operator()(EffectType type) const noexcept {
      return static_cast<std::size_t>(type);
    }
  };

  using EntityStateMap = std::unordered_map<std::uint32_t, EntityAudioState>;

  void Play(EffectType effect);
  void HandleCreation(const EntityAudioState& state);
  void HandleUpdate(const EntityAudioState& previous,
                    const EntityAudioState& current);
  void HandleRemoval(const EntityAudioState& previous);
  EntityStateMap SnapshotEntities(const engine::ecs::Registry& registry) const;
  static bool IsPlayer(std::uint16_t type_code);
  static bool IsEnemy(std::uint16_t type_code);
  static bool IsMissile(std::uint16_t type_code);
  static bool IsDamageable(const EntityAudioState& state);
  static bool IsExplosive(const EntityAudioState& state);

  engine::audio::AudioEngine& engine_;
  std::unordered_map<EffectType, std::string, EffectHasher> effect_paths_;
  EntityStateMap previous_entities_;
};

}  // namespace client

#endif  // CLIENT_SOUND_EFFECTS_H_
