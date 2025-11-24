#include "../../include/engine/input.h"

#include <algorithm>

namespace engine::input {

void InputManager::bind_key(const std::string& action, Key key) {
  auto& binding = bindings_[action];
  auto& keys = binding.keys;

  if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
    keys.push_back(key);
    key_to_actions_[key].push_back(action);
  }
  action_states_.try_emplace(action, false);
}

void InputManager::bind_mouse_button(const std::string& action,
                                     MouseButton button) {
  auto& binding = bindings_[action];
  auto& buttons = binding.mouse_buttons;

  if (std::find(buttons.begin(), buttons.end(), button) == buttons.end()) {
    buttons.push_back(button);
    mouse_to_actions_[button].push_back(action);
  }
  action_states_.try_emplace(action, false);
}

void InputManager::unbind_action(const std::string& action) {
  auto binding_it = bindings_.find(action);
  if (binding_it == bindings_.end()) return;

  for (Key key : binding_it->second.keys) {
    auto it = key_to_actions_.find(key);
    if (it != key_to_actions_.end()) {
      auto& actions = it->second;
      actions.erase(std::remove(actions.begin(), actions.end(), action),
                    actions.end());
      if (actions.empty()) key_to_actions_.erase(it);
    }
  }

  for (MouseButton button : binding_it->second.mouse_buttons) {
    auto it = mouse_to_actions_.find(button);
    if (it != mouse_to_actions_.end()) {
      auto& actions = it->second;
      actions.erase(std::remove(actions.begin(), actions.end(), action),
                    actions.end());
      if (actions.empty()) mouse_to_actions_.erase(it);
    }
  }

  bindings_.erase(binding_it);

  auto state_it = action_states_.find(action);
  if (state_it != action_states_.end()) {
    if (state_it->second) {
      events_.push_back({action, ActionEventType::kReleased});
    }
    action_states_.erase(state_it);
  }
}

void InputManager::reset_bindings() {
  bindings_.clear();
  key_to_actions_.clear();
  mouse_to_actions_.clear();

  for (auto& [action, active] : action_states_) {
    if (active) events_.push_back({action, ActionEventType::kReleased});
  }
  action_states_.clear();
}

void InputManager::handle_key(Key key, bool pressed) {
  key_states_[key] = pressed;

  auto it = key_to_actions_.find(key);
  if (it == key_to_actions_.end()) return;

  for (const auto& action : it->second) {
    const bool was_active = is_action_active(action);
    const bool now_active = compute_action_active(action);
    if (was_active != now_active) {
      action_states_[action] = now_active;
      events_.push_back({action, now_active ? ActionEventType::kPressed
                                            : ActionEventType::kReleased});
    }
  }
}

void InputManager::handle_mouse_button(MouseButton button, bool pressed) {
  mouse_states_[button] = pressed;

  auto it = mouse_to_actions_.find(button);
  if (it == mouse_to_actions_.end()) return;

  for (const auto& action : it->second) {
    const bool was_active = is_action_active(action);
    const bool now_active = compute_action_active(action);
    if (was_active != now_active) {
      action_states_[action] = now_active;
      events_.push_back({action, now_active ? ActionEventType::kPressed
                                            : ActionEventType::kReleased});
    }
  }
}

bool InputManager::is_action_active(const std::string& action) const {
  auto it = action_states_.find(action);
  if (it == action_states_.end()) return false;
  return it->second;
}

bool InputManager::is_key_down(Key key) const {
  auto it = key_states_.find(key);
  if (it == key_states_.end()) return false;
  return it->second;
}

bool InputManager::is_mouse_button_down(MouseButton button) const {
  auto it = mouse_states_.find(button);
  if (it == mouse_states_.end()) return false;
  return it->second;
}

std::vector<ActionEvent> InputManager::consume_events() {
  std::vector<ActionEvent> output;
  output.swap(events_);
  return output;
}

void InputManager::clear_state() {
  key_states_.clear();
  mouse_states_.clear();

  for (auto& [action, active] : action_states_) {
    if (active) {
      events_.push_back({action, ActionEventType::kReleased});
    }
    active = false;
  }
}

bool InputManager::compute_action_active(const std::string& action) const {
  auto binding_it = bindings_.find(action);
  if (binding_it == bindings_.end()) return false;

  const auto& binding = binding_it->second;

  for (Key key : binding.keys) {
    auto state = key_states_.find(key);
    if (state != key_states_.end() && state->second) return true;
  }

  for (MouseButton button : binding.mouse_buttons) {
    auto state = mouse_states_.find(button);
    if (state != mouse_states_.end() && state->second) return true;
  }

  return false;
}

}  // namespace engine::input
