/**
 * @file client_constants
 * @brief Client runtime constants
 */

#ifndef CLIENT_CONSTANTS_CLIENT_CONSTANTS_H_
#define CLIENT_CONSTANTS_CLIENT_CONSTANTS_H_

#include <string_view>

#include "engine/math/vector2.h"
#include "engine/render/color.h"

namespace client::constants::client {

inline const engine::math::Vector2i kBaseResolution{1600, 900};
inline constexpr engine::render::Color kClearColor =
    engine::render::Color::FromBytes(12, 12, 16);
inline constexpr float kDisconnectFadeSeconds = 1.25f;
inline constexpr float kGameOverFadeSeconds = 1.5f;
inline constexpr int kTargetFps = 60;
inline constexpr bool kWindowResizable = false;
inline constexpr bool kWindowVsync = true;
inline constexpr std::string_view kWindowTitle = "R-Type Client";
inline constexpr std::string_view kKeyBindingsPath = "config/keybindings.json";

}  // namespace client::constants::client

#endif  // CLIENT_CONSTANTS_CLIENT_CONSTANTS_H_
