#include "rift/systems/hit_detection_system.h"

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "rift/components/fighter_component.h"

namespace rift::systems {

namespace {

struct AABB {
  float min_x, max_x, min_y, max_y;

  bool Overlaps(const AABB& other) const {
    return min_x < other.max_x && max_x > other.min_x &&
           min_y < other.max_y && max_y > other.min_y;
  }
};

AABB ComputeHitbox(const engine::ecs::PositionComponent& pos,
                   const components::HitboxComponent& hitbox,
                   const components::FighterComponent& fighter) {
  float offset_x = fighter.facing_right ? hitbox.offset_x : -hitbox.offset_x - hitbox.width;
  return {
      pos.position.x + offset_x,
      pos.position.x + offset_x + hitbox.width,
      pos.position.y + hitbox.offset_y,
      pos.position.y + hitbox.offset_y + hitbox.height
  };
}

AABB ComputeHurtbox(const engine::ecs::PositionComponent& pos,
                    const components::HurtboxComponent& hurtbox) {
  return {
      pos.position.x + hurtbox.offset_x - hurtbox.width / 2.0f,
      pos.position.x + hurtbox.offset_x + hurtbox.width / 2.0f,
      pos.position.y + hurtbox.offset_y,
      pos.position.y + hurtbox.offset_y + hurtbox.height
  };
}

}  // namespace

void HitDetectionSystem::Update(engine::ecs::Registry& registry,
                                engine::time::TimeDelta) {
  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();
  auto& fighters = registry.GetComponents<components::FighterComponent>();
  auto& attacks = registry.GetComponents<components::AttackComponent>();
  auto& hitboxes = registry.GetComponents<components::HitboxComponent>();
  auto& hurtboxes = registry.GetComponents<components::HurtboxComponent>();
  auto& healths = registry.GetComponents<components::HealthComponent>();
  auto& blocks = registry.GetComponents<components::BlockComponent>();
  auto& combat_states = registry.GetComponents<components::CombatStateComponent>();

  struct FighterData {
    std::size_t idx;
    engine::ecs::PositionComponent* pos;
    components::FighterComponent* fighter;
    components::AttackComponent* attack;
    components::HitboxComponent* hitbox;
    components::HurtboxComponent* hurtbox;
    components::HealthComponent* health;
    components::BlockComponent* block;
    components::CombatStateComponent* combat;
  };

  std::vector<FighterData> fighter_data;

  for (auto [idx, pos, fighter, attack, hitbox, hurtbox, health, block, combat] :
       engine::ecs::IndexedZipper(positions, fighters, attacks, hitboxes,
                                   hurtboxes, healths, blocks, combat_states)) {
    fighter_data.push_back({idx, &(*pos), &(*fighter), &(*attack),
                           &(*hitbox), &(*hurtbox), &(*health),
                           &(*block), &(*combat)});
  }

  for (auto& attacker : fighter_data) {
    if (!attacker.hitbox->active) continue;
    if (attacker.attack->hit_confirmed) continue;

    AABB attack_box = ComputeHitbox(*attacker.pos, *attacker.hitbox, *attacker.fighter);

    for (auto& defender : fighter_data) {
      if (defender.idx == attacker.idx) continue;
      if (defender.health->invulnerable) continue;
      if (defender.combat->state == components::CombatState::kDodging) continue;

      AABB defend_box = ComputeHurtbox(*defender.pos, *defender.hurtbox);

      if (attack_box.Overlaps(defend_box)) {
        attacker.attack->hit_confirmed = true;

        std::uint32_t damage = attacker.attack->damage;

        if (defender.block->is_blocking) {
          damage = static_cast<std::uint32_t>(
              static_cast<float>(damage) * (1.0f - defender.block->damage_reduction));
        } else {
          defender.combat->state = components::CombatState::kStunned;
          defender.combat->state_timer_ms = 0;
          defender.combat->stun_duration_ms =
              attacker.attack->type == components::AttackType::kLight ? 200 : 400;
        }

        defender.health->TakeDamage(damage);
        break;
      }
    }
  }
}

}  // namespace rift::systems
