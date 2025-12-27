/**
 * @file key_binding_service
 * @brief Key binding persistence and validation
 */

#ifndef CLIENT_INPUT_KEY_BINDING_SERVICE_H_
#define CLIENT_INPUT_KEY_BINDING_SERVICE_H_

#include <filesystem>
#include <optional>
#include <string>

#include "engine/input.h"
#include "input/key_bindings.h"

namespace client {

/**
 * @enum KeyBindingUpdateStatus
 * @brief Result status for binding updates
 */
enum class KeyBindingUpdateStatus { kUpdated, kConflict, kSaveFailed };

/**
 * @struct KeyBindingUpdateResult
 * @brief Result data for a binding update
 */
struct KeyBindingUpdateResult {
  KeyBindingUpdateStatus status;
  std::string message;
};

/**
 * @class KeyBindingService
 * @brief Key binding persistence and conflict handling
 */
class KeyBindingService {
 public:
  /**
   * @brief Create the service with default path
   */
  KeyBindingService();

  /**
   * @brief Load bindings from disk
   */
  void Load();

  /**
   * @brief Access current bindings
   */
  const KeyBindings& bindings() const { return bindings_; }

  /**
   * @brief Status message for idle UI
   */
  std::string IdleMessage() const;

  /**
   * @brief Status message for binding prompt
   * @param action Action being rebound
   */
  std::string PromptMessage(GameAction action) const;

  /**
   * @brief Status message for canceled rebind
   */
  std::string CancelMessage() const;

  /**
   * @brief Update a binding and persist it
   * @param action Action to update
   * @param key New key
   */
  KeyBindingUpdateResult UpdateBinding(GameAction action,
                                       engine::input::Key key);

 private:
  bool Save();
  std::optional<GameAction> FindConflict(GameAction action,
                                         engine::input::Key key) const;
  std::string BuildConflictMessage(GameAction conflict) const;
  std::string BuildUpdateMessage(GameAction action,
                                 engine::input::Key key) const;

  KeyBindings bindings_{KeyBindings::Default()};
  std::filesystem::path path_{};
};

}  // namespace client

#endif  // CLIENT_INPUT_KEY_BINDING_SERVICE_H_
