#include "input/input_coordinator.h"

#include "constants/input_constants.h"
#include "engine/input.h"
#include "input/input_sender.h"
#include "world_update_receiver.h"

namespace client {

InputCoordinator::InputCoordinator() = default;

InputCoordinator::~InputCoordinator() = default;

void InputCoordinator::Initialize(engine::input::InputManager& input,
                                  WorldUpdateReceiver& receiver) {
  input_layer_ = std::make_unique<InputLayer>(input);
  key_binding_service_.Load();
  if (input_layer_) {
    input_layer_->ApplyBindings(key_binding_service_.bindings());
  }
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

KeyBindingUpdateResult InputCoordinator::UpdateKeyBinding(
    GameAction action, engine::input::Key key) {
  const auto result = key_binding_service_.UpdateBinding(action, key);
  if (input_layer_ && result.status != KeyBindingUpdateStatus::kConflict) {
    input_layer_->ApplyBindings(key_binding_service_.bindings());
  }
  return result;
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

}  // namespace client
