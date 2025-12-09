#include "game_instance.h"

namespace server {

GameInstance::GameInstance(std::uint32_t seed) : rng_(seed) {}

void GameInstance::OnPlayerJoined(std::uint32_t player_id) {
  auto& logger = engine::util::Logger::Default();
  logger.Info("[GameInstance] Player joined: ", player_id);

  PlayerState state{};
  state.player_id = player_id;
  state.connected = true;

  players_[player_id] = state;
}

void GameInstance::OnPlayerLeft(std::uint32_t player_id) {
  auto& logger = engine::util::Logger::Default();
  logger.Info("[GameInstance] Player left: ", player_id);

  players_.erase(player_id);
}

void GameInstance::OnPlayerInput(std::uint32_t player_id,
                                 const protocol::InputStatePayload& payload,
                                 const protocol::Header& header) {
  auto it = players_.find(player_id);
  if (it == players_.end()) {
    auto& logger = engine::util::Logger::Default();
    logger.Warn("[GameInstance] Input from unknown player: ", player_id);
    return;
  }

  PlayerState& state = it->second;

  if (payload.command_count == 0 ||
      payload.command_count > protocol::kMaxInputSequenceHistory) {
    auto& logger = engine::util::Logger::Default();
    logger.Warn(
        "[GameInstance] Invalid InputStatePayload from player ", player_id,
        " (command_count=", static_cast<int>(payload.command_count), ")");
    return;
  }

  const protocol::InputCommand* newest = nullptr;

  for (std::uint8_t i = 0; i < payload.command_count; ++i) {
    const protocol::InputCommand& cmd = payload.commands[i];

    if (cmd.input_sequence <= state.last_applied_sequence) {
      continue;
    }
    if (newest == nullptr || cmd.input_sequence > newest->input_sequence) {
      newest = &cmd;
    }
  }

  if (newest == nullptr) {
    auto& logger = engine::util::Logger::Default();
    logger.Trace(
        "[GameInstance] All input commands already applied for player ",
        player_id);
    return;
  }
  state.last_command = *newest;
  state.last_applied_sequence = newest->input_sequence;
  state.last_input_client_time_ms = newest->client_time_ms;
  state.last_input_server_time_ms = header.timestamp_ms;
  auto& logger = engine::util::Logger::Default();
  logger.Trace("[GameInstance] Updated input for player ", player_id,
               " seq=", newest->input_sequence,
               " buttons=", static_cast<int>(newest->buttons),
               " ax=", newest->analog_x, " ay=", newest->analog_y);
}

void GameInstance::Update(const engine::time::TimeDelta& delta) {
  (void)delta;

  // Placeholder for now.
  //
  // Later, this method will:
  //  - consume PlayerState::last_command for each player
  //  - update entities via gamelogic & engine ECS
  //  - handle collisions, projectiles, enemy waves, etc.
}

}  // namespace server
