/**
 * @file key_bindings.h
 * @brief Serializable key binding configuration for gameplay actions
 */

#ifndef CLIENT_KEY_BINDINGS_H_
#define CLIENT_KEY_BINDINGS_H_

#include <array>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <cstddef>

#include "engine/input.h"
#include "input/input_layer.h"

namespace client {

/**
 * @class KeyBindings
 * @brief Stores and persists key mappings for gameplay actions
 */
class KeyBindings {
 public:
  /**
   * @brief Build a key binding set with the default control scheme
   */
  static KeyBindings Default();

  /**
   * @brief Load mappings from a configuration file
   */
  bool LoadFromFile(const std::filesystem::path& path);

  /**
   * @brief Persist mappings to a configuration file
   */
  bool SaveToFile(const std::filesystem::path& path) const;

  /**
   * @brief Retrieve the primary key bound to an action
   */
  engine::input::Key Primary(GameAction action) const;

  /**
   * @brief Append a key binding without removing existing ones
   */
  void Add(GameAction action, engine::input::Key key);

  /**
   * @brief Replace bindings for a specific action with a single key
   */
  void Set(GameAction action, engine::input::Key key);

  /**
   * @brief All keys bound to the provided action
   */
  const std::vector<engine::input::Key>& KeysFor(GameAction action) const;

  /**
   * @brief Enumerate actions in display order
   */
  std::vector<GameAction> Actions() const;

 private:
  std::array<std::vector<engine::input::Key>, kGameActionCount> bindings_{};
  bool LoadFromJson(const std::filesystem::path& path);
};

/**
 * @brief Stable index for an action used by binding tables
 */
constexpr std::size_t ActionIndex(GameAction action) {
  switch (action) {
    case GameAction::kMoveUp:
      return 0;
    case GameAction::kMoveDown:
      return 1;
    case GameAction::kMoveLeft:
      return 2;
    case GameAction::kMoveRight:
      return 3;
    case GameAction::kShoot:
      return 4;
    case GameAction::kBigShoot:
      return 5;
    case GameAction::kReconnect:
      return 6;
    case GameAction::kPing:
      return 7;
  }
  return 0;
}

/**
 * @brief Human readable label for a gameplay action
 */
std::string ActionLabel(GameAction action);

/**
 * @brief Stable token used for serialization of a gameplay action
 */
std::string_view ActionToken(GameAction action);

/**
 * @brief Serialize a key to a configuration token
 */
std::string KeyToToken(engine::input::Key key);

/**
 * @brief Human readable key display string
 */
std::string KeyDisplayName(engine::input::Key key);

/**
 * @brief Parse a key token from configuration
 */
std::optional<engine::input::Key> ParseKeyToken(std::string_view token);

/**
 * @brief All recognized keyboard keys for binding
 */
std::span<const engine::input::Key> BindableKeys();

}  // namespace client

#endif  // CLIENT_KEY_BINDINGS_H_
