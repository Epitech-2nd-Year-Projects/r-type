#include "input_layer.h"

#include <array>
#include <string>

#include "key_bindings.h"

namespace client {
namespace {

constexpr std::string_view kMoveUpAction = "MoveUp";
constexpr std::string_view kMoveDownAction = "MoveDown";
constexpr std::string_view kMoveLeftAction = "MoveLeft";
constexpr std::string_view kMoveRightAction = "MoveRight";
constexpr std::string_view kShootAction = "Shoot";
constexpr std::string_view kBigShootAction = "BigShoot";
constexpr std::string_view kReconnectAction = "Reconnect";

enum MappingIndex : std::size_t {
  kMoveUpIndex = 0,
  kMoveDownIndex = 1,
  kMoveLeftIndex = 2,
  kMoveRightIndex = 3,
  kShootIndex = 4,
  kBigShootIndex = 5,
  kReconnectIndex = 6
};

struct Mapping {
  GameAction action;
  std::string_view name;
};

constexpr std::array<Mapping, 7> kMappings{
    Mapping{GameAction::kMoveUp, kMoveUpAction},        // kMoveUpIndex
    Mapping{GameAction::kMoveDown, kMoveDownAction},    // kMoveDownIndex
    Mapping{GameAction::kMoveLeft, kMoveLeftAction},    // kMoveLeftIndex
    Mapping{GameAction::kMoveRight, kMoveRightAction},  // kMoveRightIndex
    Mapping{GameAction::kShoot, kShootAction},          // kShootIndex
    Mapping{GameAction::kBigShoot, kBigShootAction},    // kBigShootIndex
    Mapping{GameAction::kReconnect, kReconnectAction}   // kReconnectIndex
};

constexpr GameActionEventType ToGameActionEventType(
    engine::input::ActionEventType type) {
  return type == engine::input::ActionEventType::kPressed
             ? GameActionEventType::kPressed
             : GameActionEventType::kReleased;
}

}  // namespace

InputLayer::InputLayer(engine::input::InputManager& manager)
    : manager_(manager) {
  for (std::size_t i = 0; i < kMappings.size(); ++i) {
    action_names_[i] = std::string(kMappings[i].name);
    action_lookup_.emplace(std::string_view(action_names_[i]),
                           kMappings[i].action);
  }
}

void InputLayer::ApplyDefaultBindings() {
  ApplyBindings(KeyBindings::Default());
}

void InputLayer::ApplyBindings(const KeyBindings& bindings) {
  auto& manager = manager_.get();
  manager.ResetBindings();

  for (const auto& mapping : kMappings) {
    const auto& keys = bindings.KeysFor(mapping.action);
    for (auto key : keys) {
      manager.BindKey(action_names_[ActionIndex(mapping.action)], key);
    }
  }
}

void InputLayer::Update() {
  auto& manager = manager_.get();
  events_.clear();
  const auto raw_events = manager.ConsumeEvents();

  for (const auto& event : raw_events) {
    const auto action = ResolveAction(event.action);
    if (!action) continue;

    events_.push_back({*action, ToGameActionEventType(event.type)});
    if (*action == GameAction::kReconnect &&
        event.type == engine::input::ActionEventType::kPressed) {
      reconnect_requested_ = true;
    }
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
  if (const auto it = action_lookup_.find(action_name);
      it != action_lookup_.end()) {
    return it->second;
  }
  return std::nullopt;
}

void InputLayer::RefreshState() {
  auto& manager = manager_.get();
  state_.move_up = manager.IsActionActive(action_names_[kMoveUpIndex]);
  state_.move_down = manager.IsActionActive(action_names_[kMoveDownIndex]);
  state_.move_left = manager.IsActionActive(action_names_[kMoveLeftIndex]);
  state_.move_right = manager.IsActionActive(action_names_[kMoveRightIndex]);
  state_.shoot = manager.IsActionActive(action_names_[kShootIndex]);
  state_.big_shoot = manager.IsActionActive(action_names_[kBigShootIndex]);
}

bool InputLayer::ConsumeReconnectRequest() {
  const bool requested = reconnect_requested_;
  reconnect_requested_ = false;
  return requested;
}

}  // namespace client
