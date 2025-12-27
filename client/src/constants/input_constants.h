/**
 * @file input_constants
 * @brief Input action names for the client
 */

#ifndef CLIENT_CONSTANTS_INPUT_CONSTANTS_H_
#define CLIENT_CONSTANTS_INPUT_CONSTANTS_H_

#include <string_view>

namespace client::constants::input {

inline constexpr std::string_view kActionConfirm = "Confirm";
inline constexpr std::string_view kActionCancel = "Cancel";
inline constexpr std::string_view kActionPause = "Pause";
inline constexpr std::string_view kActionToggleReady = "ToggleReady";

inline constexpr std::string_view kActionMoveUp = "MoveUp";
inline constexpr std::string_view kActionMoveDown = "MoveDown";
inline constexpr std::string_view kActionMoveLeft = "MoveLeft";
inline constexpr std::string_view kActionMoveRight = "MoveRight";
inline constexpr std::string_view kActionShoot = "Shoot";
inline constexpr std::string_view kActionBigShoot = "BigShoot";
inline constexpr std::string_view kActionReconnect = "Reconnect";

}  // namespace client::constants::input

#endif  // CLIENT_CONSTANTS_INPUT_CONSTANTS_H_
