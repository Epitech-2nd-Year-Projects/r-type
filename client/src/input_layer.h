/**
 * @file input_layer.h
 * @brief Client side keyboard mapping layer
 */

#ifndef CLIENT_INPUT_LAYER_H_
#define CLIENT_INPUT_LAYER_H_

#include <optional>
#include <functional>
#include <string_view>
#include <vector>

#include "engine/input.h"

namespace client {

/**
 * @enum GameAction
 * @brief High level gameplay actions exposed to the client
 */
enum class GameAction {
  kMoveUp,
  kMoveDown,
  kMoveLeft,
  kMoveRight,
  kShoot,
  kReconnect
};

/**
 * @enum GameActionEventType
 * @brief Button transitions for a gameplay action
 */
enum class GameActionEventType { kPressed, kReleased };

/**
 * @brief Action transition emitted by the input layer
 */
struct GameActionEvent {
  GameAction action;
  GameActionEventType type;
};

/**
 * @brief Snapshot of current gameplay input state
 */
struct ActionState {
  bool move_up{false};
  bool move_down{false};
  bool move_left{false};
  bool move_right{false};
  bool shoot{false};
};

/**
 * @class InputLayer
 * @brief Binds keyboard input to gameplay actions using the engine input API
 */
class InputLayer {
 public:
  /**
   * @brief Construct an input layer bound to an engine input manager
   */
  explicit InputLayer(engine::input::InputManager& manager);

  /**
   * @brief Install the default keyboard control scheme
   */
  void ApplyDefaultBindings();

  /**
   * @brief Consume engine input events and update gameplay action state
   */
  void Update();

  /**
   * @brief Retrieve a copy of the current action state
   */
  ActionState state() const { return state_; }

  /**
   * @brief Retrieve and clear translated gameplay action events
   */
 std::vector<GameActionEvent> ConsumeEvents();

  /**
   * @brief Retrieve and clear any reconnect request
   */
  bool ConsumeReconnectRequest();

 private:
  std::optional<GameAction> ResolveAction(std::string_view action_name) const;
  void RefreshState();

  std::reference_wrapper<engine::input::InputManager> manager_;
  ActionState state_{};
  std::vector<GameActionEvent> events_{};
  bool reconnect_requested_{false};
};

}  // namespace client

#endif  // CLIENT_INPUT_LAYER_H_
