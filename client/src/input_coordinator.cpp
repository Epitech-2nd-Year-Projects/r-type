#include "input_coordinator.h"

#include <utility>

#include "constants/client_constants.h"
#include "constants/input_constants.h"
#include "engine/input.h"
#include "input_sender.h"
#include "join_flow.h"
#include "logging.h"
#include "world_update_receiver.h"

namespace client {

InputCoordinator::InputCoordinator()
    : key_bindings_(KeyBindings::Default()),
      keybindings_path_(std::string(constants::client::kKeyBindingsPath)) {}

InputCoordinator::~InputCoordinator() = default;

void InputCoordinator::Initialize(engine::input::InputManager& input,
                                  WorldUpdateReceiver& receiver) {
  input_layer_ = std::make_unique<InputLayer>(input);
  LoadKeyBindings();
  input_sender_ = std::make_unique<InputSender>(*input_layer_, receiver);
  BindUiActions(input);
}

void InputCoordinator::Update(engine::time::TimeDelta dt, JoinState join_state,
                              ClientState state) {
  if (input_layer_) {
    input_layer_->Update();
  }
  if (input_sender_) {
    const bool accepts_input =
        join_state == JoinState::kConnected && state == ClientState::kInGame;
    input_sender_->Update(dt, accepts_input);
  }
}

bool InputCoordinator::ShouldReconnect(JoinState join_state,
                                       bool input_captured) {
  if (input_captured) {
    return false;
  }

  const bool request =
      input_layer_ ? input_layer_->ConsumeReconnectRequest() : false;
  if (request) {
    reconnect_requested_ = true;
  }

  const bool can_retry = join_state != JoinState::kConnected &&
                         join_state != JoinState::kConnecting;
  if (reconnect_requested_ && can_retry) {
    reconnect_requested_ = false;
    return true;
  }
  return false;
}

void InputCoordinator::ResetSender() {
  if (input_sender_) {
    input_sender_->Reset();
  }
}

void InputCoordinator::ResetReconnect() { reconnect_requested_ = false; }

bool InputCoordinator::UpdateKeyBinding(GameAction action,
                                        engine::input::Key key) {
  key_bindings_.Set(action, key);
  if (input_layer_) {
    input_layer_->ApplyBindings(key_bindings_);
  }
  return SaveKeyBindings();
}

void InputCoordinator::BindUiActions(engine::input::InputManager& input) {
  input.BindKey(std::string(constants::input::kActionConfirm),
                engine::input::Key::kEnter);
  input.BindKey(std::string(constants::input::kActionCancel),
                engine::input::Key::kEscape);
  input.BindKey(std::string(constants::input::kActionPause),
                engine::input::Key::kP);
  input.BindKey(std::string(constants::input::kActionToggleReady),
                engine::input::Key::kR);
}

void InputCoordinator::LoadKeyBindings() {
  KeyBindings bindings = KeyBindings::Default();
  if (!bindings.LoadFromFile(keybindings_path_)) {
    LogLifecycle(engine::util::LogLevel::kDebug,
                 "Key bindings config not found, applying defaults");
  }
  key_bindings_ = std::move(bindings);
  if (input_layer_) {
    input_layer_->ApplyBindings(key_bindings_);
  }
}

bool InputCoordinator::SaveKeyBindings() {
  if (!key_bindings_.SaveToFile(keybindings_path_)) {
    LogLifecycle(
        engine::util::LogLevel::kWarn,
        "Failed to persist key bindings to " + keybindings_path_.string());
    return false;
  }
  return true;
}

}  // namespace client
