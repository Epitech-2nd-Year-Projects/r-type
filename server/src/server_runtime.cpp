#include "server_runtime.h"

#include <asio/post.hpp>
#include <asio/thread_pool.hpp>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <latch>
#include <limits>
#include <string>
#include <string_view>
#include <thread>

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/tag_component.h"
#include "engine/net/endpoint.h"
#include "engine/net/packet_buffer.h"
#include "engine/scripting/prefab_factory.h"
#include "engine/scripting/script_engine.h"
#include "protocol/command.h"
#include "protocol/join.h"
#include "protocol/message_type.h"
#include "protocol/packet.h"
#include "protocol/reliability_policy.h"
#include "protocol/world_snapshot.h"
#include "server_runtime_helpers.h"

namespace server {

using namespace runtime_helpers;

namespace {

std::atomic_bool g_shutdown_requested{false};
std::atomic_bool g_dump_metrics_requested{false};
std::atomic_bool g_dump_sessions_requested{false};
std::atomic_bool g_reload_config_requested{false};
std::atomic_bool g_force_shutdown_requested{false};

void SignalHandler(int) { g_shutdown_requested.store(true); }
void MetricsSignalHandler(int) { g_dump_metrics_requested.store(true); }
void SessionsSignalHandler(int) { g_dump_sessions_requested.store(true); }
void ReloadSignalHandler(int) { g_reload_config_requested.store(true); }
void ForceShutdownHandler(int) {
  g_force_shutdown_requested.store(true);
  g_shutdown_requested.store(true);
}

void InstallSignalHandlers() {
  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);
#ifdef SIGUSR1
  std::signal(SIGUSR1, MetricsSignalHandler);
#endif
#ifdef SIGUSR2
  std::signal(SIGUSR2, SessionsSignalHandler);
#endif
#ifdef SIGHUP
  std::signal(SIGHUP, ReloadSignalHandler);
#endif
#ifdef SIGQUIT
  std::signal(SIGQUIT, ForceShutdownHandler);
#endif
}

std::size_t ComputeWorkerCount() {
  const auto hw = std::thread::hardware_concurrency();
  return hw == 0 ? 1u : static_cast<std::size_t>(hw);
}

}  // namespace

ServerRuntime::ServerRuntime(ServerConfig config)
    : config_(std::move(config)),
      logger_(engine::util::Logger::Default()),
      frame_timer_(static_cast<float>(config_.tick_rate)),
      worker_count_(ComputeWorkerCount()),
      worker_pool_(worker_count_),
      rng_(config_.seed),
      fixed_delta_(engine::time::TimeDelta::from_seconds(
          1.0f /
          static_cast<float>(config_.tick_rate > 0 ? config_.tick_rate : 60))),
      accumulator_(engine::time::TimeDelta::zero()),
      console_sink_(std::make_shared<engine::util::ConsoleSink>()),
      file_sink_(std::make_shared<engine::util::FileSink>("server.log")),
      start_time_(std::chrono::steady_clock::now()) {}

std::error_code ServerRuntime::Start() {
  ConfigureLogging();
  InstallSignalHandlers();

  if (const auto start_error = transport_.Start(config_.port); start_error) {
    return start_error;
  }

  logger_.Info("Server listening on ", transport_.local_endpoint().address());
  logger_.Info("Max players ", config_.max_players, " tickrate ",
               config_.tick_rate, " timeout_ms ", config_.peer_timeout_ms,
               " room_idle_ms ", config_.room_idle_timeout_ms, " seed ",
               config_.seed);
  logger_.Info("Worker threads ", worker_count_);
  return {};
}

void ServerRuntime::Run() { RunMainLoop(); }

void ServerRuntime::RunMainLoop() {
  running_ = true;
  accumulator_ = engine::time::TimeDelta::zero();
  const auto start_ms = NowMilliseconds();
  last_decode_metrics_log_ms_ = start_ms;
  last_server_stats_log_ms_ = start_ms;
  last_tick_health_sample_ms_ = start_ms;
  last_tick_health_sample_tick_ = server_tick_;
  frame_time_accumulator_ms_ = 0.0;
  frame_time_samples_ = 0;

  while (running_ && !g_shutdown_requested.load()) {
    const auto delta = frame_timer_.tick();
    frame_time_accumulator_ms_ += static_cast<double>(delta.as_milliseconds());
    ++frame_time_samples_;
    accumulator_ += delta;
    PollNetwork();
    ProcessAdminTasks();
    if (!running_) break;
    ProcessReliableResends();

    while (running_ && accumulator_ >= fixed_delta_) {
      PollNetwork();
      if (!running_) break;
      ProcessReliableResends();
      UpdateRoomsParallel(fixed_delta_);
      ++server_tick_;
      BroadcastGameEvents();
      BroadcastWorldSnapshots();
      accumulator_ -= fixed_delta_;
    }
    CheckPeerTimeouts();
    reassembler_.Cleanup(NowMilliseconds(), 5000);
    PruneOrphanedSessions();
    MaybeLogDecodeMetrics();
    MaybeLogServerStats();
    if (g_force_shutdown_requested.exchange(false)) {
      logger_.Warn("Force shutdown requested");
      running_ = false;
      break;
    }
    if (g_dump_sessions_requested.exchange(false)) {
      DumpSessions();
    }
    if (g_reload_config_requested.exchange(false)) {
      ReloadConfiguration();
    }
    TickRateSleep(delta);
  }
  LogDecodeMetricsSummary(true);
  running_ = false;
  transport_.Stop();
}

void ServerRuntime::ConfigureLogging() {
  logger_.SetName("server");
  logger_.SetLevel(config_.log_level);
  logger_.ClearSinks();
  logger_.AddSink(console_sink_);
  logger_.AddSink(file_sink_);
}

void ServerRuntime::TickRateSleep(const engine::time::TimeDelta& delta_time) {
  const float target_ms =
      1000.0f /
      static_cast<float>(config_.tick_rate > 0 ? config_.tick_rate : 1);
  const float delta_ms = delta_time.as_milliseconds();
  if (delta_ms >= target_ms) {
    return;
  }
  const auto sleep_ms =
      std::chrono::milliseconds(static_cast<int>(target_ms - delta_ms));
  if (sleep_ms.count() > 0) {
    std::this_thread::sleep_for(sleep_ms);
  }
}

void ServerRuntime::MaybeLogDecodeMetrics() {
  const auto now_ms = NowMilliseconds();
  const bool force_dump = g_dump_metrics_requested.exchange(false);
  const auto elapsed_ms =
      ElapsedMilliseconds(last_decode_metrics_log_ms_, now_ms);
  if (!force_dump && elapsed_ms < kDecodeMetricsLogIntervalMs) {
    return;
  }
  last_decode_metrics_log_ms_ = now_ms;
  LogDecodeMetricsSummary(force_dump);
}

void ServerRuntime::MaybeLogServerStats() {
  const auto now_ms = NowMilliseconds();
  const auto elapsed_ms =
      ElapsedMilliseconds(last_server_stats_log_ms_, now_ms);
  if (elapsed_ms < kServerDiagnosticsLogIntervalMs) {
    return;
  }

  std::size_t joined_peers = 0;
  std::size_t connecting_peers = 0;
  std::size_t disconnected_peers = 0;
  for (const auto& [_, peer] : peers_) {
    switch (peer.state) {
      case PeerState::kJoined:
        ++joined_peers;
        break;
      case PeerState::kConnecting:
        ++connecting_peers;
        break;
      case PeerState::kDisconnected:
        ++disconnected_peers;
        break;
    }
  }

  const auto health_window_ms =
      ElapsedMilliseconds(last_tick_health_sample_ms_, now_ms);
  const auto ticks_sampled = server_tick_ >= last_tick_health_sample_tick_
                                 ? server_tick_ - last_tick_health_sample_tick_
                                 : 0;
  const double elapsed_seconds =
      health_window_ms > 0 ? static_cast<double>(health_window_ms) / 1000.0
                           : 0.0;
  const double actual_tick_rate =
      elapsed_seconds > 0.0
          ? static_cast<double>(ticks_sampled) / elapsed_seconds
          : 0.0;
  const double target_tick_rate =
      static_cast<double>(config_.tick_rate > 0 ? config_.tick_rate : 1);
  const double health_ratio =
      target_tick_rate > 0.0 ? actual_tick_rate / target_tick_rate : 0.0;
  const bool tick_healthy = health_ratio >= kTickHealthWarningThreshold;
  const double avg_frame_ms = frame_time_samples_ > 0
                                  ? frame_time_accumulator_ms_ /
                                        static_cast<double>(frame_time_samples_)
                                  : 0.0;

  logger_.Info("Diagnostics peers total=", peers_.size(),
               " joined=", joined_peers, " connecting=", connecting_peers,
               " disconnected=", disconnected_peers,
               " players=", players_.size(), " rooms=", rooms_.size(),
               " tickrate target=", config_.tick_rate,
               " actual=", actual_tick_rate, " avg_frame_ms=", avg_frame_ms,
               " backlog_ms=", accumulator_.as_milliseconds(),
               " health=", tick_healthy ? "ok" : "degraded");

  for (const auto& [room_code, room] : rooms_) {
    logger_.Info("Room ", room_code, " [",
                 (room.IsPrivate() ? "private" : "public"), "] players ",
                 room.PlayerCount(), "/", room.MaxPlayers());
  }

  last_server_stats_log_ms_ = now_ms;
  last_tick_health_sample_ms_ = now_ms;
  last_tick_health_sample_tick_ = server_tick_;
  frame_time_accumulator_ms_ = 0.0;
  frame_time_samples_ = 0;
}

void ServerRuntime::UpdateRoomsParallel(const engine::time::TimeDelta& delta) {
  if (rooms_.empty()) {
    return;
  }

  std::vector<std::reference_wrapper<Room>> rooms_snapshot;
  rooms_snapshot.reserve(rooms_.size());
  for (auto& [_, room] : rooms_) {
    rooms_snapshot.push_back(std::ref(room));
  }

  if (worker_count_ <= 1 || rooms_snapshot.size() == 1) {
    for (auto& room_ref : rooms_snapshot) {
      room_ref.get().Update(delta);
    }
    return;
  }

  std::latch latch(static_cast<std::ptrdiff_t>(rooms_snapshot.size()));
  for (auto room_ref : rooms_snapshot) {
    asio::post(worker_pool_, [room_ref, delta, &latch, this]() {
      try {
        room_ref.get().Update(delta);
      } catch (const std::exception& ex) {
        logger_.Error("Room update failed: ", ex.what());
      } catch (...) {
        logger_.Error("Room update failed with unknown error");
      }
      latch.count_down();
    });
  }
  latch.wait();
}

void ServerRuntime::DumpSessions() {
  logger_.Info("Session dump peers=", peers_.size(),
               " players=", players_.size(), " rooms=", rooms_.size(),
               " tick=", server_tick_);

  for (const auto& [room_code, room] : rooms_) {
    logger_.Info("Room ", room_code, " id=", room.Id(), " players ",
                 room.PlayerCount(), "/", room.MaxPlayers(),
                 " privacy=", (room.IsPrivate() ? "private" : "public"),
                 " seed=", room.Seed());
  }

  for (const auto& [endpoint, peer] : peers_) {
    logger_.Info("Peer ", endpoint, " state=", PeerStateToString(peer.state),
                 " player_id=", peer.player_id, " room=", peer.room_code);
  }
}

void ServerRuntime::ReloadConfiguration() {
  const char* env_log_level = std::getenv("RTYPE_SERVER_LOG_LEVEL");
  if (env_log_level != nullptr) {
    config_.log_level =
        engine::util::ParseLogLevel(env_log_level, config_.log_level);
  }
  ConfigureLogging();
  logger_.Info("Configuration reloaded log_level=",
               engine::util::ToString(config_.log_level));
}

void ServerRuntime::LogDecodeMetricsSummary(bool force) {
  const auto& m = decode_metrics_;
  if (!force && m.total_packets == 0) {
    return;
  }
  logger_.Info("DecodeMetrics total=", m.total_packets,
               " rejected=", m.rejected_packets,
               " errors[unknown_message_type=", m.unknown_message_type,
               " version_mismatch=", m.version_mismatch,
               " unexpected_end_of_buffer=", m.unexpected_end_of_buffer,
               " invalid_header=", m.invalid_header,
               " invalid_payload=", m.invalid_payload,
               " invalid_snapshot_id=", m.invalid_snapshot_id, "]");
}

void ServerRuntime::ProcessAdminTasks() {
  std::function<void(ServerRuntime&)> task;
  while (admin_tasks_.TryPop(task)) {
    try {
      task(*this);
    } catch (const std::exception& e) {
      logger_.Error("Admin task failed: ", e.what());
    }
  }
}

void ServerRuntime::EnqueueAdminTask(std::function<void(ServerRuntime&)> task) {
  admin_tasks_.Push(std::move(task));
}

void ServerRuntime::RequestShutdown() { g_shutdown_requested.store(true); }

void ServerRuntime::SetConsoleLogsEnabled(bool enabled) {
  if (console_sink_) {
    console_sink_->SetEnabled(enabled);
  }
}

bool ServerRuntime::ConsoleLogsEnabled() const {
  return console_sink_ && console_sink_->IsEnabled();
}

const std::unordered_map<std::string, PeerConnection>& ServerRuntime::Peers()
    const {
  return peers_;
}

std::unordered_map<std::string, PeerConnection>& ServerRuntime::Peers() {
  return peers_;
}

const std::unordered_map<std::string, Room>& ServerRuntime::Rooms() const {
  return rooms_;
}

std::unordered_map<std::string, Room>& ServerRuntime::Rooms() { return rooms_; }

const std::unordered_map<std::uint32_t, ServerRuntime::PlayerSession>&
ServerRuntime::Players() const {
  return players_;
}

std::uint32_t ServerRuntime::ServerTick() const { return server_tick_; }

const ServerConfig& ServerRuntime::Config() const { return config_; }

std::chrono::steady_clock::time_point ServerRuntime::StartTime() const {
  return start_time_;
}

engine::util::Logger& ServerRuntime::Logger() { return logger_; }

bool ServerRuntime::KickPlayer(std::uint32_t player_id) {
  auto peer_opt = FindPeerByPlayerId(player_id);
  if (!peer_opt.has_value()) {
    return false;
  }
  RemovePeer(peer_opt->get());
  return true;
}

std::size_t ServerRuntime::RemoveEnemiesFromRoom(const std::string& room_code) {
  auto room_opt = FindRoom(room_code);
  if (!room_opt.has_value()) {
    return 0;
  }
  auto& room = room_opt->get();
  auto& registry = room.World();

  std::vector<engine::ecs::EntityId> to_kill;
  auto& tags = registry.GetComponents<engine::ecs::TagComponent>();

  for (std::size_t i = 0; i < tags.size(); ++i) {
    if (tags[i].has_value()) {
      const auto& tag = tags[i]->tag;
      if (tag == "Enemy" || tag == "enemy" || tag == "enemy_scout" ||
          tag == "enemy_bomber" || tag == "boss" || tag == "enemy_basic") {
        to_kill.push_back(registry.EntityFromIndex(i));
      }
    }
  }

  for (const auto& entity : to_kill) {
    registry.KillEntity(entity);
  }

  return to_kill.size();
}

bool ServerRuntime::SpawnEntityInRoom(const std::string& room_code,
                                      const std::string& prefab_name, float x,
                                      float y) {
  auto room_opt = FindRoom(room_code);
  if (!room_opt.has_value()) {
    return false;
  }
  auto& room = room_opt->get();
  auto& logic = room.Logic();
  auto& registry = room.World();
  auto& prefab_factory = logic.ScriptEngine().GetPrefabFactory();

  auto entity_opt = prefab_factory.Spawn(registry, prefab_name);
  if (!entity_opt.has_value()) {
    logger_.Warn("Spawn failed: unknown prefab '", prefab_name, "'");
    return false;
  }

  std::size_t entity_idx = static_cast<std::size_t>(*entity_opt);
  auto& positions = registry.GetComponents<engine::ecs::PositionComponent>();

  if (entity_idx < positions.size() && positions[entity_idx].has_value()) {
    positions[entity_idx]->position.x = x;
    positions[entity_idx]->position.y = y;
  } else {
    registry.EmplaceComponent<engine::ecs::PositionComponent>(*entity_opt, x,
                                                              y);
  }

  logger_.Info("Spawned '", prefab_name, "' at (", x, ", ", y, ") in room ",
               room_code);
  return true;
}

std::vector<std::string> ServerRuntime::GetAvailableEntities(
    const std::string& room_code) {
  auto room_opt = FindRoom(room_code);
  if (!room_opt.has_value()) {
    return {};
  }
  return room_opt->get()
      .Logic()
      .ScriptEngine()
      .GetPrefabFactory()
      .GetAvailablePrefabs();
}

}  // namespace server
