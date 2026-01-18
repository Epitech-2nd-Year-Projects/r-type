#include "animation_system.h"

#include <algorithm>
#include <cmath>

#include "ecs/components.h"
#include "engine/util/logging.h"

namespace rift::client {

namespace {

const char* kMovementAnimationsPath =
    "rift/client/assets/animations/Rig_Medium_MovementBasic.glb";
const char* kCombatMeleeAnimationsPath =
    "rift/client/assets/animations/Rig_Medium_CombatMelee.glb";
const char* kSpecialAnimationsPath =
    "rift/client/assets/animations/Rig_Medium_Special.glb";

enum class AnimationSource { kMovement, kCombatMelee, kSpecial };

struct AnimationMapping {
  rift::components::CombatState state;
  const char* name;
  bool looping;
  float speed;
  AnimationSource source;
};

constexpr AnimationMapping kAnimationMappings[] = {
    {rift::components::CombatState::kIdle, "Melee_Unarmed_Idle", true, 1.0f,
     AnimationSource::kCombatMelee},
    {rift::components::CombatState::kWalking, "Walking_A", true, 1.0f,
     AnimationSource::kMovement},
    {rift::components::CombatState::kAttacking, "Melee_1H_Attack_Slice_Diagonal",
     false, 1.5f, AnimationSource::kCombatMelee},
    {rift::components::CombatState::kBlocking, "Melee_Blocking", true, 1.0f,
     AnimationSource::kCombatMelee},
    {rift::components::CombatState::kStunned, "Melee_Block_Hit", false, 1.0f,
     AnimationSource::kCombatMelee},
    {rift::components::CombatState::kDodging, "Jump_Full_Short", false, 2.0f,
     AnimationSource::kMovement},
};

}  // namespace

AnimationSystem::AnimationSystem(engine::render::Renderer3D& renderer)
    : renderer_(renderer) {}

AnimationSystem::~AnimationSystem() = default;

bool AnimationSystem::Initialize() {
  movement_animations_ =
      renderer_.LoadAnimationsFromFile(kMovementAnimationsPath);
  combat_melee_animations_ =
      renderer_.LoadAnimationsFromFile(kCombatMeleeAnimationsPath);
  special_animations_ = renderer_.LoadAnimationsFromFile(kSpecialAnimationsPath);

  if ((!movement_animations_ || movement_animations_->GetCount() == 0) &&
      (!combat_melee_animations_ || combat_melee_animations_->GetCount() == 0) &&
      (!special_animations_ || special_animations_->GetCount() == 0)) {
    engine::util::Logger::Default().Warn(
        "AnimationSystem: No animations loaded, using static models");
    return false;
  }

  auto log_animations = [](const char* name,
                           engine::render::AnimationSet* set) {
    if (!set) return;
    engine::util::Logger::Default().Info(name, " animations:");
    for (std::size_t i = 0; i < set->GetCount(); ++i) {
      const auto* anim = set->GetAnimation(i);
      if (anim) {
        engine::util::Logger::Default().Info("  [", i, "] ", anim->GetName(),
                                             " (", anim->GetFrameCount(),
                                             " frames)");
      }
    }
  };

  log_animations("Movement", movement_animations_.get());
  log_animations("CombatMelee", combat_melee_animations_.get());
  log_animations("Special", special_animations_.get());

  auto get_set = [this](AnimationSource source) -> engine::render::AnimationSet* {
    switch (source) {
      case AnimationSource::kMovement:
        return movement_animations_.get();
      case AnimationSource::kCombatMelee:
        return combat_melee_animations_.get();
      case AnimationSource::kSpecial:
        return special_animations_.get();
    }
    return nullptr;
  };

  auto source_name = [](AnimationSource source) -> const char* {
    switch (source) {
      case AnimationSource::kMovement:
        return "movement";
      case AnimationSource::kCombatMelee:
        return "combat_melee";
      case AnimationSource::kSpecial:
        return "special";
    }
    return "unknown";
  };

  for (const auto& mapping : kAnimationMappings) {
    auto* set = get_set(mapping.source);
    if (!set) continue;

    const auto* anim = set->FindByName(mapping.name);
    if (anim) {
      for (std::size_t i = 0; i < set->GetCount(); ++i) {
        if (set->GetAnimation(i) == anim) {
          state_to_animation_[mapping.state] = AnimationRef{set, i};
          engine::util::Logger::Default().Info(
              "Mapped state ", static_cast<int>(mapping.state), " -> ",
              mapping.name, " (", source_name(mapping.source), ")");
          break;
        }
      }
    } else {
      engine::util::Logger::Default().Warn("Animation '", mapping.name,
                                           "' not found in ",
                                           source_name(mapping.source), " set");
    }
  }

  if (state_to_animation_.find(rift::components::CombatState::kIdle) ==
      state_to_animation_.end()) {
    if (movement_animations_ && movement_animations_->GetCount() > 0) {
      state_to_animation_[rift::components::CombatState::kIdle] =
          AnimationRef{movement_animations_.get(), 0};
      engine::util::Logger::Default().Info(
          "Using animation 0 as default for idle");
    }
  }

  return !state_to_animation_.empty();
}

void AnimationSystem::Update(engine::ecs::Registry& registry,
                             engine::time::TimeDelta dt,
                             const FightActionState& input_state,
                             std::optional<std::uint32_t> local_player_id) {
  if (state_to_animation_.empty()) return;

  UpdateCombatDisplayStates(registry, input_state, local_player_id);

  auto& fighters = registry.GetComponents<ecs::FighterStateComponent>();
  auto& animations = registry.GetComponents<ecs::AnimationComponent>();
  auto& combat_displays = registry.GetComponents<ecs::CombatDisplayComponent>();

  for (std::size_t i = 0; i < fighters.size(); ++i) {
    if (!fighters[i].has_value()) continue;

    if (!animations[i].has_value()) {
      animations[i] = ecs::AnimationComponent{};

      auto it = state_to_animation_.find(rift::components::CombatState::kIdle);
      if (it != state_to_animation_.end()) {
        animations[i]->animation_set = GetSharedAnimationSet(it->second.set);
        animations[i]->current_animation_index = it->second.index;
      }
    }

    rift::components::CombatState current_state =
        rift::components::CombatState::kIdle;
    if (i < combat_displays.size() && combat_displays[i].has_value()) {
      current_state = combat_displays[i]->state;
    }

    auto last_it = last_combat_state_.find(i);
    if (last_it == last_combat_state_.end() ||
        last_it->second != current_state) {
      TransitionToAnimation(i, registry, current_state);
      last_combat_state_[i] = current_state;
    }

    UpdateEntityAnimation(i, registry, dt);
  }
}

void AnimationSystem::UpdateCombatDisplayStates(
    engine::ecs::Registry& registry, const FightActionState& input_state,
    std::optional<std::uint32_t> local_player_id) {
  auto& fighters = registry.GetComponents<ecs::FighterStateComponent>();
  auto& velocities = registry.GetComponents<ecs::VelocityComponent>();
  auto& combat_displays = registry.GetComponents<ecs::CombatDisplayComponent>();

  for (std::size_t i = 0; i < fighters.size(); ++i) {
    if (!fighters[i].has_value()) continue;

    if (!combat_displays[i].has_value()) {
      combat_displays[i] = ecs::CombatDisplayComponent{};
    }

    auto& combat = combat_displays[i].value();
    const bool is_local_player =
        local_player_id.has_value() &&
        fighters[i]->player_id == local_player_id.value();

    rift::components::CombatState new_state = rift::components::CombatState::kIdle;

    if (is_local_player) {
      if (input_state.light_attack || input_state.heavy_attack) {
        new_state = rift::components::CombatState::kAttacking;
      } else if (input_state.block) {
        new_state = rift::components::CombatState::kBlocking;
      } else if (input_state.dodge) {
        new_state = rift::components::CombatState::kDodging;
      } else if (input_state.move_left || input_state.move_right) {
        new_state = rift::components::CombatState::kWalking;
      }
    }

    if (!is_local_player || new_state == rift::components::CombatState::kIdle) {
      if (i < velocities.size() && velocities[i].has_value()) {
        const auto& vel = velocities[i]->velocity;
        if (std::abs(vel.x) > 1.0f) {
          new_state = rift::components::CombatState::kWalking;
        }
      }
    }

    combat.state = new_state;
  }
}

void AnimationSystem::UpdateEntityAnimation(std::size_t entity_index,
                                            engine::ecs::Registry& registry,
                                            engine::time::TimeDelta dt) {
  auto& animations = registry.GetComponents<ecs::AnimationComponent>();
  auto& anim = animations[entity_index];
  if (!anim.has_value() || !anim->playing || !anim->animation_set) return;

  const auto* current_anim =
      anim->animation_set->GetAnimation(anim->current_animation_index);
  if (!current_anim) return;

  const float frame_advance =
      dt.as_seconds() * kDefaultAnimationFps * anim->speed;
  anim->current_frame += frame_advance;

  const float total_frames =
      static_cast<float>(current_anim->GetFrameCount());

  if (anim->current_frame >= total_frames) {
    if (anim->looping) {
      anim->current_frame = std::fmod(anim->current_frame, total_frames);
    } else {
      anim->current_frame = total_frames - 1.0f;
      anim->playing = false;
    }
  }
}

void AnimationSystem::TransitionToAnimation(
    std::size_t entity_index, engine::ecs::Registry& registry,
    rift::components::CombatState new_state) {
  auto& animations = registry.GetComponents<ecs::AnimationComponent>();
  auto& anim = animations[entity_index];
  if (!anim.has_value()) return;

  auto it = state_to_animation_.find(new_state);
  if (it == state_to_animation_.end()) {
    it = state_to_animation_.find(rift::components::CombatState::kIdle);
    if (it == state_to_animation_.end()) return;
  }

  for (const auto& mapping : kAnimationMappings) {
    if (mapping.state == new_state ||
        (it->first == rift::components::CombatState::kIdle &&
         mapping.state == rift::components::CombatState::kIdle)) {
      anim->animation_set = GetSharedAnimationSet(it->second.set);
      anim->current_animation_index = it->second.index;
      anim->current_frame = 0.0f;
      anim->looping = mapping.looping;
      anim->speed = mapping.speed;
      anim->playing = true;
      break;
    }
  }
}

std::shared_ptr<engine::render::AnimationSet>
AnimationSystem::GetSharedAnimationSet(engine::render::AnimationSet* raw_ptr) {
  if (raw_ptr == movement_animations_.get()) {
    return movement_animations_;
  }
  if (raw_ptr == combat_melee_animations_.get()) {
    return combat_melee_animations_;
  }
  if (raw_ptr == special_animations_.get()) {
    return special_animations_;
  }
  return nullptr;
}

void AnimationSystem::ApplyAnimations(engine::ecs::Registry& registry) {
  if (state_to_animation_.empty()) return;

  auto& animations = registry.GetComponents<ecs::AnimationComponent>();
  auto& render3d = registry.GetComponents<ecs::Fighter3DRenderComponent>();

  for (std::size_t i = 0; i < animations.size(); ++i) {
    if (!animations[i].has_value() || !render3d[i].has_value()) continue;
    if (!render3d[i]->model || !animations[i]->animation_set) continue;

    const auto* anim = animations[i]->animation_set->GetAnimation(
        animations[i]->current_animation_index);
    if (!anim) continue;

    const std::uint32_t frame =
        static_cast<std::uint32_t>(animations[i]->current_frame);

    renderer_.UpdateModelAnimation(*render3d[i]->model, *anim, frame);
  }
}

}  // namespace rift::client
