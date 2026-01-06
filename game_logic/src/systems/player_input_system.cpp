#include "game_logic/systems/player_input_system.h"

#include <utility>

#include "engine/math/vector2.h"
#include "game_logic/game_instance.h"

namespace {

void ApplyInputToVelocity(game_logic::GameInstance::InputEventType type,
                          game_logic::GameInstance::InputState &input_state,
                          engine::ecs::VelocityComponent &velocity,
                          float speed) {
  using InputType = game_logic::GameInstance::InputEventType;

  switch (type) {
    case InputType::kMoveLeftPressed:
      input_state.move_left = true;
      break;
    case InputType::kMoveLeftReleased:
      input_state.move_left = false;
      break;
    case InputType::kMoveRightPressed:
      input_state.move_right = true;
      break;
    case InputType::kMoveRightReleased:
      input_state.move_right = false;
      break;
    case InputType::kMoveUpPressed:
      input_state.move_up = true;
      break;
    case InputType::kMoveUpReleased:
      input_state.move_up = false;
      break;
    case InputType::kMoveDownPressed:
      input_state.move_down = true;
      break;
    case InputType::kMoveDownReleased:
      input_state.move_down = false;
      break;
    case InputType::kBasicShootPressed:
    case InputType::kBasicShootReleased:
    case InputType::kBigShootPressed:
    case InputType::kBigShootReleased:
      break;
  }

  engine::math::Vector2f &v = velocity.velocity;

  v.x = 0.0f;
  v.y = 0.0f;

  if (input_state.move_left && !input_state.move_right) {
    v.x = -speed;
  } else if (input_state.move_right && !input_state.move_left) {
    v.x = speed;
  }

  if (input_state.move_up && !input_state.move_down) {
    v.y = -speed;
  } else if (input_state.move_down && !input_state.move_up) {
    v.y = speed;
  }
}

}  // namespace

namespace game_logic::systems {

void PlayerInputSystem::Update(
    engine::ecs::Registry &,
    engine::ecs::SparseArray<components::PlayerComponent> &players,
    engine::ecs::SparseArray<engine::ecs::VelocityComponent> &velocities,
    engine::ecs::SparseArray<components::WeaponComponent> &weapons,
    engine::ecs::SparseArray<components::ShootEventComponent> &shoot_events,
    engine::time::TimeDelta,
    std::reference_wrapper<GameInstance> instance_ref) {
  GameInstance &instance = instance_ref.get();

  if (instance.pending_inputs_.empty()) {
    return;
  }

  using InputType = GameInstance::InputEventType;

  for (const auto &evt : instance.pending_inputs_) {
    auto entity_it = instance.player_entities_.find(evt.player_id);
    if (entity_it == instance.player_entities_.end()) {
      continue;
    }

    auto input_it = instance.player_input_states_.find(evt.player_id);
    if (input_it == instance.player_input_states_.end()) {
      continue;
    }

    std::size_t index = static_cast<std::size_t>(entity_it->second);
    if (index >= players.size() || index >= velocities.size() ||
        index >= weapons.size()) {
      continue;
    }

    auto &player_opt = players[index];
    auto &velocity_opt = velocities[index];
    auto &weapon_opt = weapons[index];
    if (!player_opt.has_value() || !velocity_opt.has_value()) {
      continue;
    }

    auto &velocity = velocity_opt.value();
    auto &input_state = input_it->second;

    ApplyInputToVelocity(evt.type, input_state, velocity,
                         GameInstance::kInputMoveSpeed);

    if (weapon_opt.has_value()) {
      auto &weapon = weapon_opt.value();
      if (evt.type == InputType::kBasicShootPressed) {
        weapon.is_trigger_held = true;

        if (evt.spawn_pos.has_value()) {
          if (index < shoot_events.size() && shoot_events[index].has_value()) {
            auto &shoot_evt = shoot_events[index].value();
            shoot_evt.fired = true;
            shoot_evt.is_big_shot = false;
            shoot_evt.spawn_position = evt.spawn_pos.value();
            shoot_evt.latency_s = evt.latency_s;
          }
        }
      } else if (evt.type == InputType::kBasicShootReleased) {
        weapon.is_trigger_held = false;
      } else if (evt.type == InputType::kBigShootPressed) {
        weapon.is_big_trigger_held = true;
      } else if (evt.type == InputType::kBigShootReleased) {
        weapon.is_big_trigger_held = false;
      }
    }
  }

  instance.pending_inputs_.clear();
}

}  // namespace game_logic::systems
