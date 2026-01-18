/**
 * @file input_coordinator.h
 * @brief Input routing and key binding control
 */

#ifndef CLIENT_INPUT_COORDINATOR_H_
#define CLIENT_INPUT_COORDINATOR_H_

#include <memory>

#include "client_state.h"
#include "engine/time/time_delta.h"
#include "input/input_layer.h"
#include "input/key_binding_service.h"
#include "join_flow.h"

namespace engine::input {
class InputManager;
}  // namespace engine::input

namespace client {

class InputSender;
class WorldUpdateReceiver;

/**
 * @brief Input coordinator for gameplay and UI actions
 */
class InputCoordinator {
 public:
  /**
   * @brief Construct the input coordinator
   */
  InputCoordinator();

  /**
   * @brief Destroy the input coordinator
   */
  ~InputCoordinator();

  InputCoordinator(const InputCoordinator&) = delete;
  InputCoordinator& operator=(const InputCoordinator&) = delete;
  InputCoordinator(InputCoordinator&&) = delete;
  InputCoordinator& operator=(InputCoordinator&&) = delete;

  /**
   * @brief Initialize input systems
   * @param input Engine input manager
   * @param receiver World update receiver
   */
  void Initialize(engine::input::InputManager& input,
                  WorldUpdateReceiver& receiver);

  /**
   * @brief Update input state and send gameplay input
   * @param dt Frame delta
   * @param join_state Current join state
   * @param state Current client state
   */
  void Update(engine::time::TimeDelta dt, JoinState join_state,
              ClientState state);

  /**
   * @brief Handle reconnect requests
   * @param join_state Current join state
   * @param input_captured Scene input capture flag
   * @return True when a reconnect should be triggered
   */
  bool ShouldReconnect(JoinState join_state, bool input_captured);

  /**
   * @brief Reset input sender state
   */
  void ResetSender();

  /**
   * @brief Clear reconnect requests
   */
  void ResetReconnect();

  /**
   * @brief Access key bindings
   */
  const KeyBindings& key_bindings() const {
    return key_binding_service_.bindings();
  }

  /**
   * @brief Access the latest action state
   */
  ActionState action_state() const;

  /**
   * @brief Access key binding service
   */
  const KeyBindingService& key_binding_service() const {
    return key_binding_service_;
  }

  /**
   * @brief Update a key binding and persist it
   * @param action Gameplay action
   * @param key Input key
   * @return Result for the binding update
   */
  KeyBindingUpdateResult UpdateKeyBinding(GameAction action,
                                          engine::input::Key key);

 private:
  void BindUiActions(engine::input::InputManager& input);

  std::unique_ptr<InputLayer> input_layer_;
  std::unique_ptr<InputSender> input_sender_;
  KeyBindingService key_binding_service_{};
  bool reconnect_requested_{false};
};

}  // namespace client

#endif  // CLIENT_INPUT_COORDINATOR_H_
