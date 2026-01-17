#include "input/fight_input.h"

#include <array>
#include <string>

namespace rift::client {

namespace {

constexpr std::string_view kActionMoveLeft = "move_left";
constexpr std::string_view kActionMoveRight = "move_right";
constexpr std::string_view kActionLightAttack = "light_attack";
constexpr std::string_view kActionHeavyAttack = "heavy_attack";
constexpr std::string_view kActionBlock = "block";
constexpr std::string_view kActionDodge = "dodge";

enum MappingIndex : std::size_t {
  kMoveLeftIndex = 0,
  kMoveRightIndex = 1,
  kLightAttackIndex = 2,
  kHeavyAttackIndex = 3,
  kBlockIndex = 4,
  kDodgeIndex = 5
};

struct Mapping {
  FightAction action;
  std::string_view name;
};

constexpr std::array<Mapping, kFightActionCount> kMappings{
    Mapping{FightAction::kMoveLeft, kActionMoveLeft},
    Mapping{FightAction::kMoveRight, kActionMoveRight},
    Mapping{FightAction::kLightAttack, kActionLightAttack},
    Mapping{FightAction::kHeavyAttack, kActionHeavyAttack},
    Mapping{FightAction::kBlock, kActionBlock},
    Mapping{FightAction::kDodge, kActionDodge}};

constexpr FightActionEventType ToFightActionEventType(
    engine::input::ActionEventType type) {
  return type == engine::input::ActionEventType::kPressed
             ? FightActionEventType::kPressed
             : FightActionEventType::kReleased;
}

constexpr std::size_t ActionIndex(FightAction action) {
  return static_cast<std::size_t>(action);
}

}  // namespace

FightInputLayer::FightInputLayer(engine::input::InputManager& manager)
    : manager_(manager) {
  for (std::size_t i = 0; i < kMappings.size(); ++i) {
    action_names_[i] = std::string(kMappings[i].name);
    action_lookup_.emplace(std::string_view(action_names_[i]),
                           kMappings[i].action);
  }
}

void FightInputLayer::ApplyDefaultBindings() {
  auto& manager = manager_.get();

  for (const auto& mapping : kMappings) {
    manager.UnbindAction(action_names_[ActionIndex(mapping.action)]);
  }

  manager.BindKey(action_names_[kMoveLeftIndex], engine::input::Key::kA);
  manager.BindKey(action_names_[kMoveRightIndex], engine::input::Key::kD);
  manager.BindKey(action_names_[kLightAttackIndex], engine::input::Key::kJ);
  manager.BindKey(action_names_[kHeavyAttackIndex], engine::input::Key::kK);
  manager.BindKey(action_names_[kBlockIndex], engine::input::Key::kL);
  manager.BindKey(action_names_[kDodgeIndex], engine::input::Key::kSpace);

  manager.BindKey(action_names_[kMoveLeftIndex], engine::input::Key::kLeft);
  manager.BindKey(action_names_[kMoveRightIndex], engine::input::Key::kRight);
}

void FightInputLayer::Update() {
  auto& manager = manager_.get();
  events_.clear();
  const auto raw_events = manager.ConsumeEvents();

  for (const auto& event : raw_events) {
    const auto action = ResolveAction(event.action);
    if (!action) continue;

    events_.push_back({*action, ToFightActionEventType(event.type)});
  }

  RefreshState();
}

std::vector<FightActionEvent> FightInputLayer::ConsumeEvents() {
  std::vector<FightActionEvent> output;
  output.swap(events_);
  return output;
}

std::optional<FightAction> FightInputLayer::ResolveAction(
    std::string_view action_name) const {
  if (const auto it = action_lookup_.find(action_name);
      it != action_lookup_.end()) {
    return it->second;
  }
  return std::nullopt;
}

void FightInputLayer::RefreshState() {
  auto& manager = manager_.get();
  state_.move_left = manager.IsActionActive(action_names_[kMoveLeftIndex]);
  state_.move_right = manager.IsActionActive(action_names_[kMoveRightIndex]);
  state_.light_attack =
      manager.IsActionActive(action_names_[kLightAttackIndex]);
  state_.heavy_attack =
      manager.IsActionActive(action_names_[kHeavyAttackIndex]);
  state_.block = manager.IsActionActive(action_names_[kBlockIndex]);
  state_.dodge = manager.IsActionActive(action_names_[kDodgeIndex]);
}

}  // namespace rift::client
