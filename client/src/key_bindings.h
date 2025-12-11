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

#include "engine/input.h"
#include "input_layer.h"

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
  static std::size_t ActionIndex(GameAction action);
  std::array<std::vector<engine::input::Key>, 6> bindings_{};
};

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
