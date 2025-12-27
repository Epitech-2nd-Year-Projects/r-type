#include "input/key_binding_service.h"

#include <utility>

#include "constants/client_constants.h"
#include "logging.h"

namespace client {

KeyBindingService::KeyBindingService()
    : path_(std::string(constants::client::kKeyBindingsPath)) {}

void KeyBindingService::Load() {
  KeyBindings bindings = KeyBindings::Default();
  if (!bindings.LoadFromFile(path_)) {
    LogLifecycle(engine::util::LogLevel::kDebug,
                 "Key bindings config not found, applying defaults");
  }
  bindings_ = std::move(bindings);
}

std::string KeyBindingService::IdleMessage() const {
  return "Click a binding to remap";
}

std::string KeyBindingService::PromptMessage(GameAction action) const {
  return "Press a key for " + ActionLabel(action);
}

std::string KeyBindingService::CancelMessage() const {
  return "Rebind canceled";
}

KeyBindingUpdateResult KeyBindingService::UpdateBinding(
    GameAction action, engine::input::Key key) {
  if (const auto conflict = FindConflict(action, key)) {
    return {KeyBindingUpdateStatus::kConflict, BuildConflictMessage(*conflict)};
  }

  bindings_.Set(action, key);
  if (!Save()) {
    return {KeyBindingUpdateStatus::kSaveFailed, "Failed to save key bindings"};
  }

  return {KeyBindingUpdateStatus::kUpdated, BuildUpdateMessage(action, key)};
}

bool KeyBindingService::Save() {
  if (!bindings_.SaveToFile(path_)) {
    LogLifecycle(engine::util::LogLevel::kWarn,
                 "Failed to persist key bindings to " + path_.string());
    return false;
  }
  return true;
}

std::optional<GameAction> KeyBindingService::FindConflict(
    GameAction action, engine::input::Key key) const {
  for (GameAction other : bindings_.Actions()) {
    if (other == action) {
      continue;
    }
    if (bindings_.Primary(other) == key) {
      return other;
    }
  }
  return std::nullopt;
}

std::string KeyBindingService::BuildConflictMessage(GameAction conflict) const {
  return "Key already bound to " + ActionLabel(conflict);
}

std::string KeyBindingService::BuildUpdateMessage(
    GameAction action, engine::input::Key key) const {
  return "Bound " + ActionLabel(action) + " to " + KeyDisplayName(key);
}

}  // namespace client
