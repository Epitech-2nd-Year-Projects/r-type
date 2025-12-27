/**
 * @file key_mappings
 * @brief Key mapping data for input and UI
 */

#ifndef ENGINE_INPUT_KEY_MAPPINGS_H_
#define ENGINE_INPUT_KEY_MAPPINGS_H_

#include <span>
#include <string_view>

#include "engine/input.h"

namespace engine::input {

/**
 * @struct KeyMapping
 * @brief Key mapping entry for tokens display and text
 */
struct KeyMapping {
  Key key;
  std::string_view token;
  std::string_view display;
  std::string_view text;
  std::string_view shifted_text;
};

/**
 * @brief List of key mappings in display order
 */
std::span<const KeyMapping> KeyMappings();

/**
 * @brief Key token used for serialization
 */
std::string_view KeyToken(Key key);

/**
 * @brief Key display name for UI
 */
std::string_view KeyDisplayName(Key key);

/**
 * @brief Key text for text input
 * @param key Key to map
 * @param shifted True when shift is held
 * @return Text mapping or empty when no mapping
 */
std::string_view KeyText(Key key, bool shifted);

}  // namespace engine::input

#endif  // ENGINE_INPUT_KEY_MAPPINGS_H_
