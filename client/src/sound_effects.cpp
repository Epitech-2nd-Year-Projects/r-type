/**
 * @file sound_effects.cpp
 * @brief Client-side sound effect routing for gameplay feedback
 */

#include "sound_effects.h"

#include <utility>

#include "audio_paths.h"
#include "ecs/components.h"
#include "logging.h"

namespace client {
namespace {

constexpr std::uint16_t kPlayerTypeCode = 1u;
constexpr std::uint16_t kEnemyTypeCode = 2u;
constexpr std::uint16_t kMissileTypeCode = 3u;
constexpr std::uint16_t kObstacleTypeCode = 4u;

}  // namespace

SoundEffects::SoundEffects(engine::audio::AudioEngine& engine)
    : engine_(engine) {
  const std::string effect_path = ResolveAssetPath("assets/fah.ogg");
  effect_paths_.emplace(EffectType::kShot, effect_path);
  effect_paths_.emplace(EffectType::kExplosion, effect_path);
  effect_paths_.emplace(EffectType::kDamage, effect_path);
  effect_paths_.emplace(EffectType::kPlayerDeath, effect_path);
}

void SoundEffects::LoadAssets() {
  const float original_volume = engine_.GetSfxVolume();
  engine_.SetSfxVolume(0.0f);
  for (const auto& [_, path] : effect_paths_) {
    if (!path.empty()) {
      engine_.PlaySoundEffect(path);
    }
  }
  engine_.SetSfxVolume(original_volume);
}

void SoundEffects::Reset() { previous_entities_.clear(); }

void SoundEffects::OnSnapshotApplied(const engine::ecs::Registry& registry) {
  EntityStateMap current = SnapshotEntities(registry);

  for (const auto& [id, state] : current) {
    const auto it = previous_entities_.find(id);
    if (it == previous_entities_.end()) {
      HandleCreation(state);
      continue;
    }
    HandleUpdate(it->second, state);
  }

  for (const auto& [id, previous] : previous_entities_) {
    if (current.find(id) != current.end()) {
      continue;
    }
    HandleRemoval(previous);
  }

  previous_entities_ = std::move(current);
}

void SoundEffects::OnPlayerDeath() { Play(EffectType::kPlayerDeath); }

void SoundEffects::Play(EffectType effect) {
  const auto it = effect_paths_.find(effect);
  if (it == effect_paths_.end() || it->second.empty()) {
    LogLifecycle(engine::util::LogLevel::kWarn,
                 "No sound effect configured for requested event");
    return;
  }
  engine_.PlaySoundEffect(it->second);
}

void SoundEffects::HandleCreation(const EntityAudioState& state) {
  if (IsMissile(state.type_code)) {
    Play(EffectType::kShot);
  }
}

void SoundEffects::HandleUpdate(const EntityAudioState& previous,
                                const EntityAudioState& current) {
  if (previous.alive && !current.alive) {
    Play(EffectType::kExplosion);
    return;
  }

  if (IsDamageable(current) && previous.has_health && current.has_health &&
      current.health < previous.health) {
    Play(EffectType::kDamage);
  }
}

void SoundEffects::HandleRemoval(const EntityAudioState& previous) {
  if (previous.alive && IsExplosive(previous)) {
    Play(EffectType::kExplosion);
  }
}

SoundEffects::EntityStateMap SoundEffects::SnapshotEntities(
    const engine::ecs::Registry& registry) const {
  EntityStateMap snapshot;

  const auto& net = registry.GetComponents<ecs::NetworkedEntityComponent>();
  const auto& health = registry.GetComponents<ecs::HealthComponent>();

  for (std::size_t i = 0; i < net.size(); ++i) {
    if (!net[i].has_value()) {
      continue;
    }
    EntityAudioState state;
    state.type_code = net[i]->type_code;
    if (i < health.size() && health[i].has_value()) {
      state.has_health = true;
      state.health = health[i]->current;
      state.alive = health[i]->current > 0;
    }
    snapshot[net[i]->network_id] = state;
  }

  return snapshot;
}

bool SoundEffects::IsPlayer(std::uint16_t type_code) {
  return type_code == kPlayerTypeCode;
}

bool SoundEffects::IsEnemy(std::uint16_t type_code) {
  return type_code == kEnemyTypeCode;
}

bool SoundEffects::IsMissile(std::uint16_t type_code) {
  return type_code == kMissileTypeCode;
}

bool SoundEffects::IsDamageable(const EntityAudioState& state) {
  return state.has_health &&
         (IsPlayer(state.type_code) || IsEnemy(state.type_code) ||
          state.type_code == kObstacleTypeCode);
}

bool SoundEffects::IsExplosive(const EntityAudioState& state) {
  return IsMissile(state.type_code) || state.has_health;
}

}  // namespace client
