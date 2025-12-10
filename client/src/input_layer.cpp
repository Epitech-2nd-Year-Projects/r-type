#include "input_layer.h"

#include <array>
#include <string>

namespace client {
namespace {

constexpr std::string_view kMoveUpAction = "MoveUp";
constexpr std::string_view kMoveDownAction = "MoveDown";
constexpr std::string_view kMoveLeftAction = "MoveLeft";
constexpr std::string_view kMoveRightAction = "MoveRight";
constexpr std::string_view kShootAction = "Shoot";

struct Mapping {
  GameAction action;
  std::string_view name;
};

constexpr std::array<Mapping, 5> kMappings{
    Mapping{GameAction::kMoveUp, kMoveUpAction},
    Mapping{GameAction::kMoveDown, kMoveDownAction},
    Mapping{GameAction::kMoveLeft, kMoveLeftAction},
    Mapping{GameAction::kMoveRight, kMoveRightAction},
    Mapping{GameAction::kShoot, kShootAction},
};

constexpr GameActionEventType ToGameActionEventType(
    engine::input::ActionEventType type) {
  return type == engine::input::ActionEventType::kPressed
             ? GameActionEventType::kPressed
             : GameActionEventType::kReleased;
}

}  // namespace

InputLayer::InputLayer(engine::input::InputManager& manager)
    : manager_(manager) {}

void InputLayer::ApplyDefaultBindings() {
  auto& manager = manager_.get();

  manager.ResetBindings();
  manager.BindKey(std::string(kMoveUpAction), engine::input::Key::kW);
  manager.BindKey(std::string(kMoveUpAction), engine::input::Key::kZ);
  manager.BindKey(std::string(kMoveUpAction), engine::input::Key::kUp);

  manager.BindKey(std::string(kMoveDownAction), engine::input::Key::kS);
  manager.BindKey(std::string(kMoveDownAction), engine::input::Key::kDown);

  manager.BindKey(std::string(kMoveLeftAction), engine::input::Key::kQ);
  manager.BindKey(std::string(kMoveLeftAction), engine::input::Key::kA);
  manager.BindKey(std::string(kMoveLeftAction), engine::input::Key::kLeft);

  manager.BindKey(std::string(kMoveRightAction), engine::input::Key::kD);
  manager.BindKey(std::string(kMoveRightAction), engine::input::Key::kRight);

  manager.BindKey(std::string(kShootAction), engine::input::Key::kSpace);
}

void InputLayer::Update() {
  auto& manager = manager_.get();
  events_.clear();
  const auto raw_events = manager.ConsumeEvents();

  for (const auto& event : raw_events) {
    const auto action = ResolveAction(event.action);
    if (!action) continue;

    events_.push_back({*action, ToGameActionEventType(event.type)});
  }

  RefreshState();
}

std::vector<GameActionEvent> InputLayer::ConsumeEvents() {
  std::vector<GameActionEvent> output;
  output.swap(events_);
  return output;
}

std::optional<GameAction> InputLayer::ResolveAction(
    std::string_view action_name) const {
  for (const auto& mapping : kMappings) {
    if (mapping.name == action_name) {
      return mapping.action;
    }
  }
  return std::nullopt;
}

void InputLayer::RefreshState() {
  auto& manager = manager_.get();
  state_.move_up = manager.IsActionActive(std::string(kMoveUpAction));
  state_.move_down = manager.IsActionActive(std::string(kMoveDownAction));
  state_.move_left = manager.IsActionActive(std::string(kMoveLeftAction));
  state_.move_right = manager.IsActionActive(std::string(kMoveRightAction));
  state_.shoot = manager.IsActionActive(std::string(kShootAction));
}

}  // namespace client
