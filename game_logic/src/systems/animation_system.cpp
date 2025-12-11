#include "game_logic/systems/animation_system.h"

#include "engine/ecs/indexed_zipper.h"
#include "game_logic/components/animation_component.h"
#include "game_logic/components/sprite_component.h"

namespace game_logic::systems {

void AnimationSystem::Update(engine::ecs::Registry &registry,
                             engine::time::TimeDelta dt) {
  auto &anim_components =
      registry.GetComponents<components::AnimationComponent>();
  auto &sprite_components =
      registry.GetComponents<components::SpriteComponent>();

  for (auto &&[idx, anim, sprite] :
       engine::ecs::IndexedZipper(anim_components, sprite_components)) {
    auto &animation = anim.value();
    auto &sprite_comp = sprite.value();

    if (!animation.playing || animation.frames.empty()) {
      continue;
    }

    if (animation.frame_duration <= engine::time::TimeDelta::zero()) {
      continue;
    }

    animation.elapsed += dt;

    if (animation.elapsed >= animation.frame_duration) {
      while (animation.elapsed >= animation.frame_duration) {
        animation.elapsed -= animation.frame_duration;
        animation.current_frame++;

        if (animation.current_frame >= animation.frames.size()) {
          if (animation.looping) {
            animation.current_frame = 0;
          } else {
            animation.current_frame = animation.frames.size() - 1;
            animation.playing = false;
            break;
          }
        }
      }
    }

    if (animation.current_frame < animation.frames.size()) {
      sprite_comp.source_rect = animation.frames[animation.current_frame];
    }
  }
}

}  // namespace game_logic::systems
