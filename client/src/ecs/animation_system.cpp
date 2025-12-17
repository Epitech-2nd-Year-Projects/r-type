#include "ecs/animation_system.h"

#include <iostream>

#include "ecs/components.h"

namespace client::ecs {

AnimationSystem::AnimationSystem(engine::ecs::Registry& registry)
    : registry_(registry) {}

void AnimationSystem::Update(engine::time::TimeDelta dt) {
  auto& sprites = registry_.GetComponents<SpriteComponent>();
  auto& anims = registry_.GetComponents<AnimationComponent>();

  const float delta_seconds = dt.as_seconds();

  for (std::size_t i = 0; i < anims.size(); ++i) {
    if (!anims[i].has_value()) {
      continue;
    }

    auto& anim = *anims[i];
    if (!anim.playing || anim.frames.empty()) {
      continue;
    }

    if (i >= sprites.size() || !sprites[i].has_value()) {
      continue;
    }

    auto& sprite = *sprites[i];

    anim.timer += delta_seconds;
    while (anim.timer >= anim.frame_duration) {
      anim.timer -= anim.frame_duration;
      anim.current_frame++;

      if (anim.current_frame >= anim.frames.size()) {
        if (anim.loop) {
          anim.current_frame = 0;
        } else {
          anim.current_frame = anim.frames.size() - 1;
          anim.playing = false;
        }
      }
    }

    if (anim.current_frame < anim.frames.size()) {
      sprite.source_rect = anim.frames[anim.current_frame];
    }
  }
}

}  // namespace client::ecs
