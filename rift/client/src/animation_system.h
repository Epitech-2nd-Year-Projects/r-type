#ifndef RIFT_CLIENT_ANIMATION_SYSTEM_H_
#define RIFT_CLIENT_ANIMATION_SYSTEM_H_

#include <memory>
#include <unordered_map>

#include "engine/ecs/registry.h"
#include "engine/render/animation.h"
#include "engine/render/renderer3d.h"
#include "engine/time/time_delta.h"
#include "input/fight_input.h"
#include "rift/components/fighter_component.h"

namespace rift::client {

/// Manages animation loading, state transitions, and frame updates.
class AnimationSystem {
 public:
  explicit AnimationSystem(engine::render::Renderer3D& renderer);
  ~AnimationSystem();

  /// Initialize the system, loading shared animation sets.
  bool Initialize();

  /// Update animation states for all animated entities.
  void Update(engine::ecs::Registry& registry, engine::time::TimeDelta dt,
              const FightActionState& input_state,
              std::optional<std::uint32_t> local_player_id);

  /// Apply animations to models before drawing.
  void ApplyAnimations(engine::ecs::Registry& registry);

 private:
  void UpdateCombatDisplayStates(engine::ecs::Registry& registry,
                                 const FightActionState& input_state,
                                 std::optional<std::uint32_t> local_player_id);

  void UpdateEntityAnimation(std::size_t entity_index,
                             engine::ecs::Registry& registry,
                             engine::time::TimeDelta dt);

  void TransitionToAnimation(std::size_t entity_index,
                             engine::ecs::Registry& registry,
                             rift::components::CombatState new_state);

  std::shared_ptr<engine::render::AnimationSet> GetSharedAnimationSet(
      engine::render::AnimationSet* raw_ptr);

  engine::render::Renderer3D& renderer_;
  std::shared_ptr<engine::render::AnimationSet> movement_animations_;
  std::shared_ptr<engine::render::AnimationSet> combat_melee_animations_;
  std::shared_ptr<engine::render::AnimationSet> special_animations_;

  struct AnimationRef {
    engine::render::AnimationSet* set;
    std::size_t index;
  };
  std::unordered_map<rift::components::CombatState, AnimationRef>
      state_to_animation_;
  std::unordered_map<std::size_t, rift::components::CombatState>
      last_combat_state_;

  static constexpr float kDefaultAnimationFps = 30.0f;
};

}  // namespace rift::client

#endif  // RIFT_CLIENT_ANIMATION_SYSTEM_H_
