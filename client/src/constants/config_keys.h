/**
 * @file config_keys
 * @brief Configuration key names for the client runtime
 */

#ifndef CLIENT_CONSTANTS_CONFIG_KEYS_H_
#define CLIENT_CONSTANTS_CONFIG_KEYS_H_

#include <string_view>

namespace client::constants::config {

inline constexpr std::string_view kClientHost = "client.host";
inline constexpr std::string_view kClientPort = "client.port";
inline constexpr std::string_view kClientDebug = "client.debug";
inline constexpr std::string_view kClientLogLevel = "client.log_level";
inline constexpr std::string_view kClientPlayerName = "client.player_name";
inline constexpr std::string_view kClientRoomCode = "client.room_code";
inline constexpr std::string_view kClientTimeoutMs = "client.timeout_ms";
inline constexpr std::string_view kClientPingIntervalMs =
    "client.ping_interval_ms";
inline constexpr std::string_view kClientQueueSize = "client.queue_size";
inline constexpr std::string_view kClientJoinRetryMs = "client.join_retry_ms";
inline constexpr std::string_view kClientJoinMaxAttempts =
    "client.join_max_attempts";
inline constexpr std::string_view kClientLobbyRetryMs = "client.lobby_retry_ms";
inline constexpr std::string_view kClientLobbyMaxAttempts =
    "client.lobby_max_attempts";
inline constexpr std::string_view kClientRoomListRefreshMs =
    "client.room_list_refresh_ms";
inline constexpr std::string_view kAudioMasterVolume = "audio.master_volume";
inline constexpr std::string_view kAudioMusicVolume = "audio.music_volume";
inline constexpr std::string_view kAudioSfxVolume = "audio.sfx_volume";
inline constexpr std::string_view kVideoResolutionWidth =
    "video.resolution_width";
inline constexpr std::string_view kVideoResolutionHeight =
    "video.resolution_height";
inline constexpr std::string_view kVideoFullscreen = "video.fullscreen";
inline constexpr std::string_view kVideoVsync = "video.vsync";
inline constexpr std::string_view kVideoTargetFps = "video.target_fps";

}  // namespace client::constants::config

#endif  // CLIENT_CONSTANTS_CONFIG_KEYS_H_
