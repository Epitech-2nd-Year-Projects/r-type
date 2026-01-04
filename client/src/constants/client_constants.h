/**
 * @file client_constants
 * @brief Client runtime constants
 */

#ifndef CLIENT_CONSTANTS_CLIENT_CONSTANTS_H_
#define CLIENT_CONSTANTS_CLIENT_CONSTANTS_H_

#include <cstddef>
#include <cstdint>
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
inline constexpr std::string_view kClientConfigPath = "config/client.json";
inline constexpr std::string_view kKeyBindingsPath = "config/keybindings.json";
inline constexpr std::string_view kVersionConfigPath = "config/version.json";
inline constexpr std::string_view kBloomShaderPath = "assets/shaders/bloom.fs";
inline constexpr float kBloomThreshold = 0.55f;
inline constexpr float kBloomKnee = 0.2f;
inline constexpr float kBloomIntensity = 0.6f;
inline constexpr std::size_t kLobbyPasswordLength = 4;
inline constexpr int kLobbyDefaultMaxPlayers = 4;
inline constexpr int kLobbyMaxPlayersMin = 1;
inline constexpr int kLobbyMaxPlayersMax = 255;
inline constexpr std::uint8_t kPlayerReadyFlag = 1u << 1;

}  // namespace client::constants::client

#endif  // CLIENT_CONSTANTS_CLIENT_CONSTANTS_H_
