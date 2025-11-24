#ifndef ENGINE_ENGINE_INPUT_H_
#define ENGINE_ENGINE_INPUT_H_

#include <string>
#include <unordered_map>
#include <vector>

namespace engine::input {

/**
 * @brief Keyboard keys supported by the input system
 */
enum class Key {
  kUnknown = -1,
  kA = 0,
  kB,
  kC,
  kD,
  kE,
  kF,
  kG,
  kH,
  kI,
  kJ,
  kK,
  kL,
  kM,
  kN,
  kO,
  kP,
  kQ,
  kR,
  kS,
  kT,
  kU,
  kV,
  kW,
  kX,
  kY,
  kZ,
  kNum0,
  kNum1,
  kNum2,
  kNum3,
  kNum4,
  kNum5,
  kNum6,
  kNum7,
  kNum8,
  kNum9,
  kUp,
  kDown,
  kLeft,
  kRight,
  kSpace,
  kEnter,
  kEscape,
  kLeftShift,
  kRightShift,
  kLeftControl,
  kRightControl,
  kLeftAlt,
  kRightAlt
};

/**
 * @brief Mouse buttons supported by the input system
 */
enum class MouseButton { kLeft = 0, kRight, kMiddle, kButton4, kButton5 };

/**
 * @brief Type of action transition emitted by the manager
 */
enum class ActionEventType { kPressed, kReleased };

/**
 * @brief High-level input event generated from raw device input
 */
struct ActionEvent {
  std::string action;
  ActionEventType type;
};

/**
 * @brief Maps raw keyboard/mouse input to high-level gameplay actions
 *
 * The manager tracks button states, exposes immediate action state queries and
 * records action transitions (pressed/released) that can be consumed each
 * frame. Bindings are string-based to keep the system engine-agnostic (e.g.
 * "MoveLeft", "Shoot").
 */
class InputManager {
 public:
  /**
   * @brief Bind an action to a keyboard key
   */
  void bind_key(const std::string& action, Key key);

  /**
   * @brief Bind an action to a mouse button
   */
  void bind_mouse_button(const std::string& action, MouseButton button);

  /**
   * @brief Remove all bindings/state for a specific action
   */
  void unbind_action(const std::string& action);

  /**
   * @brief Remove every binding from the manager
   */
  void reset_bindings();

  /**
   * @brief Feed a raw keyboard state change
   * @param key The key that changed
   * @param pressed True when pressed, false when released
   */
  void handle_key(Key key, bool pressed);

  /**
   * @brief Feed a raw mouse button state change
   * @param button The button that changed
   * @param pressed True when pressed, false when released
   */
  void handle_mouse_button(MouseButton button, bool pressed);

  /**
   * @brief Query whether an action is currently active
   */
  bool is_action_active(const std::string& action) const;

  /**
   * @brief Query whether a key is currently held
   */
  bool is_key_down(Key key) const;

  /**
   * @brief Query whether a mouse button is currently held
   */
  bool is_mouse_button_down(MouseButton button) const;

  /**
   * @brief Retrieve and clear pending action events
   *
   * Returns a copy of the current transition events and empties the queue.
   */
  std::vector<ActionEvent> consume_events();

  /**
   * @brief Reset device state and mark all actions as released
   *
   * Useful when focus is lost: ensures no "stuck" inputs remain active.
   */
  void clear_state();

 private:
  struct EnumClassHash {
    template <typename T>
    std::size_t operator()(T value) const noexcept {
      return static_cast<std::size_t>(value);
    }
  };

  struct ActionBinding {
    std::vector<Key> keys;
    std::vector<MouseButton> mouse_buttons;
  };

  bool compute_action_active(const std::string& action) const;

  std::unordered_map<std::string, ActionBinding> bindings_;
  std::unordered_map<std::string, bool> action_states_;
  std::unordered_map<Key, bool, EnumClassHash> key_states_;
  std::unordered_map<MouseButton, bool, EnumClassHash> mouse_states_;
  std::unordered_map<Key, std::vector<std::string>, EnumClassHash>
      key_to_actions_;
  std::unordered_map<MouseButton, std::vector<std::string>, EnumClassHash>
      mouse_to_actions_;
  std::vector<ActionEvent> events_;
};

}  // namespace engine::input

#endif /* !ENGINE_ENGINE_INPUT_H_ */
