#include "scene_manager.h"

#include <array>
#include <string_view>
#include <utility>

#include "client_context.h"
#include "logging.h"
#include "scene/connecting_scene.h"
#include "scene/disconnected_scene.h"
#include "scene/game_over_scene.h"
#include "scene/in_game_scene.h"
#include "scene/lobby_scene.h"
#include "scene/main_menu_scene.h"
#include "scene/options_menu_scene.h"
#include "scene/pause_scene.h"
#include "scene/profile_scene.h"
#include "scene/splash_scene.h"

namespace client {
namespace {

struct TransitionRule {
  ClientState from;
  ClientState to;
  bool (*guard)(const SceneManager&, ClientState);
};

bool AllowAlways(const SceneManager&, ClientState) { return true; }

bool AllowSettingsReturn(const SceneManager& manager, ClientState next_state) {
  return manager.settings_return_state().has_value() &&
         manager.settings_return_state().value() == next_state;
}

constexpr std::array<TransitionRule, 36> kTransitionRules{{
    {ClientState::kSplash, ClientState::kMainMenu, &AllowAlways},
    {ClientState::kMainMenu, ClientState::kConnecting, &AllowAlways},
    {ClientState::kMainMenu, ClientState::kLobby, &AllowAlways},
    {ClientState::kMainMenu, ClientState::kSettings, &AllowAlways},
    {ClientState::kMainMenu, ClientState::kProfile, &AllowAlways},
    {ClientState::kMainMenu, ClientState::kMainMenu, &AllowAlways},
    {ClientState::kMainMenu, ClientState::kDisconnected, &AllowAlways},
    {ClientState::kProfile, ClientState::kMainMenu, &AllowAlways},
    {ClientState::kLobby, ClientState::kConnecting, &AllowAlways},
    {ClientState::kLobby, ClientState::kSettings, &AllowAlways},
    {ClientState::kLobby, ClientState::kMainMenu, &AllowAlways},
    {ClientState::kLobby, ClientState::kDisconnected, &AllowAlways},
    {ClientState::kSettings, ClientState::kMainMenu, &AllowAlways},
    {ClientState::kSettings, ClientState::kLobby, &AllowAlways},
    {ClientState::kSettings, ClientState::kPaused, &AllowAlways},
    {ClientState::kSettings, ClientState::kInGame, &AllowAlways},
    {ClientState::kSettings, ClientState::kConnecting, &AllowSettingsReturn},
    {ClientState::kSettings, ClientState::kGameOver, &AllowSettingsReturn},
    {ClientState::kSettings, ClientState::kDisconnected, &AllowSettingsReturn},
    {ClientState::kConnecting, ClientState::kInGame, &AllowAlways},
    {ClientState::kConnecting, ClientState::kDisconnected, &AllowAlways},
    {ClientState::kConnecting, ClientState::kMainMenu, &AllowAlways},
    {ClientState::kConnecting, ClientState::kLobby, &AllowAlways},
    {ClientState::kInGame, ClientState::kPaused, &AllowAlways},
    {ClientState::kInGame, ClientState::kGameOver, &AllowAlways},
    {ClientState::kInGame, ClientState::kDisconnected, &AllowAlways},
    {ClientState::kPaused, ClientState::kInGame, &AllowAlways},
    {ClientState::kPaused, ClientState::kSettings, &AllowAlways},
    {ClientState::kPaused, ClientState::kGameOver, &AllowAlways},
    {ClientState::kPaused, ClientState::kMainMenu, &AllowAlways},
    {ClientState::kPaused, ClientState::kDisconnected, &AllowAlways},
    {ClientState::kGameOver, ClientState::kMainMenu, &AllowAlways},
    {ClientState::kGameOver, ClientState::kDisconnected, &AllowAlways},
    {ClientState::kDisconnected, ClientState::kConnecting, &AllowAlways},
    {ClientState::kDisconnected, ClientState::kMainMenu, &AllowAlways},
}};

std::string_view ToString(ClientState state) {
  switch (state) {
    case ClientState::kSplash:
      return "Splash";
    case ClientState::kMainMenu:
      return "MainMenu";
    case ClientState::kProfile:
      return "Profile";
    case ClientState::kLobby:
      return "Lobby";
    case ClientState::kSettings:
      return "Settings";
    case ClientState::kConnecting:
      return "Connecting";
    case ClientState::kInGame:
      return "InGame";
    case ClientState::kPaused:
      return "Paused";
    case ClientState::kGameOver:
      return "GameOver";
    case ClientState::kDisconnected:
      return "Disconnected";
  }
  return "Unknown";
}

}  // namespace

SceneManager::SceneManager(ClientContext& context) : context_(context) {}

SceneManager::~SceneManager() = default;

void SceneManager::Initialize(ClientState initial_state) {
  ApplyState(initial_state, {});
  CommitSceneChange();
}

void SceneManager::Update(engine::time::TimeDelta dt) {
  if (current_scene_) {
    current_scene_->Update(dt);
  }
}

void SceneManager::Draw(engine::render::Renderer2D& renderer) {
  if (current_scene_) {
    current_scene_->Draw(renderer);
  }
}

void SceneManager::CommitSceneChange() {
  if (pending_scene_) {
    current_scene_ = std::move(pending_scene_);
  }
}

bool SceneManager::IsInputCaptured() const {
  return current_scene_ && current_scene_->IsInputCaptured();
}

void SceneManager::OnPlay() { TransitionTo(ClientState::kLobby); }

void SceneManager::OnConnected() { TransitionTo(ClientState::kInGame); }

void SceneManager::OnConnectionFailed(const std::string& reason) {
  TransitionTo(ClientState::kDisconnected, reason);
}

void SceneManager::OnGameStart() { TransitionTo(ClientState::kInGame); }

void SceneManager::OnGamePause() { TransitionTo(ClientState::kPaused); }

void SceneManager::OnGameResume() { TransitionTo(ClientState::kInGame); }

void SceneManager::OnGameOver() { OnGameOver(GameOverStats{}); }

void SceneManager::OnGameOver(const GameOverStats& stats) {
  if (state_ == ClientState::kGameOver) {
    return;
  }
  last_game_stats_ = stats;
  TransitionTo(ClientState::kGameOver);
}

void SceneManager::OnDisconnect(std::string reason) {
  TransitionTo(ClientState::kDisconnected, std::move(reason));
}

void SceneManager::OnQuitToMenu() { TransitionTo(ClientState::kMainMenu); }

void SceneManager::OnOpenSettings() {
  if (state_ == ClientState::kSettings) {
    return;
  }
  settings_return_state_ = state_;
  TransitionTo(ClientState::kSettings);
}

void SceneManager::OnCloseSettings() {
  const ClientState target =
      settings_return_state_.value_or(ClientState::kMainMenu);
  settings_return_state_.reset();
  if (TransitionTo(target)) {
    return;
  }
  TransitionTo(ClientState::kMainMenu);
}

void SceneManager::OnOpenProfile() {
  if (state_ == ClientState::kProfile) {
    return;
  }
  TransitionTo(ClientState::kProfile);
}

void SceneManager::OnCloseProfile() {
  TransitionTo(ClientState::kMainMenu);
}

bool SceneManager::TransitionTo(ClientState next_state, std::string reason) {
  if (!CanTransition(next_state)) {
    LogLifecycle(engine::util::LogLevel::kWarn,
                 "Rejected state transition " + std::string(ToString(state_)) +
                     " -> " + std::string(ToString(next_state)));
    return false;
  }

  ApplyState(next_state, std::move(reason));
  return true;
}

bool SceneManager::CanTransitionTo(ClientState next_state) const {
  return CanTransition(next_state);
}

void SceneManager::SwitchScene(std::shared_ptr<Scene> scene) {
  pending_scene_ = std::move(scene);
}

void SceneManager::ApplyState(ClientState next_state, std::string reason) {
  if (next_state == ClientState::kDisconnected) {
    disconnect_reason_ =
        reason.empty() ? "Disconnected from server" : std::move(reason);
  } else {
    disconnect_reason_.clear();
  }

  const bool scene_missing = !current_scene_;
  const bool state_changed = state_ != next_state;
  state_ = next_state;

  const bool refresh_scene =
      scene_missing || state_changed || state_ == ClientState::kDisconnected;
  if (!refresh_scene) {
    return;
  }

  switch (state_) {
    case ClientState::kSplash:
      SwitchScene(std::make_shared<SplashScene>(context_));
      break;
    case ClientState::kMainMenu:
      SwitchScene(std::make_shared<MainMenuScene>(context_));
      break;
    case ClientState::kProfile:
      SwitchScene(std::make_shared<ProfileScene>(context_));
      break;
    case ClientState::kLobby:
      SwitchScene(std::make_shared<LobbyScene>(context_));
      break;
    case ClientState::kSettings:
      SwitchScene(std::make_shared<OptionsMenuScene>(context_));
      break;
    case ClientState::kConnecting:
      SwitchScene(std::make_shared<ConnectingScene>(context_));
      break;
    case ClientState::kInGame:
      SwitchScene(std::make_shared<InGameScene>(context_));
      break;
    case ClientState::kPaused:
      SwitchScene(std::make_shared<PauseScene>(context_));
      break;
    case ClientState::kGameOver: {
      GameOverScene::Stats scene_stats;
      scene_stats.score = last_game_stats_.score;
      scene_stats.wave = last_game_stats_.wave;
      SwitchScene(std::make_shared<GameOverScene>(context_, scene_stats));
      break;
    }
    case ClientState::kDisconnected:
      SwitchScene(
          std::make_shared<DisconnectedScene>(context_, disconnect_reason_));
      break;
  }
}

bool SceneManager::CanTransition(ClientState next_state) const {
  for (const auto& rule : kTransitionRules) {
    if (rule.from == state_ && rule.to == next_state) {
      if (!rule.guard || rule.guard(*this, next_state)) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace client
