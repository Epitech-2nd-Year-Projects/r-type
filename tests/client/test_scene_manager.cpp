#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "client_asset_manager.h"
#include "client_context.h"
#include "engine/audio/audio_engine.h"
#include "engine/ecs/registry.h"
#include "engine/input.h"
#include "engine/render/context.h"
#include "engine/render/renderer2d.h"
#include "engine/render/window.h"
#include "engine/util/config.h"
#include "input/key_binding_service.h"
#include "scene/lobby_scene.h"
#include "scene/main_menu_scene.h"
#include "scene/options_menu_scene.h"
#include "scene_manager.h"
#include "ui/menu_background.h"

namespace {

class FakeTexture final : public engine::render::Texture2D {
 public:
  explicit FakeTexture(engine::math::Vector2i size) : size_(size) {}

  engine::math::Vector2i GetSize() const override { return size_; }

 private:
  engine::math::Vector2i size_;
};

class FakeRenderer final : public engine::render::Renderer2D {
 public:
  void DrawRect(const engine::math::RectF& /*rect*/,
                const engine::render::Color& /*color*/) override {}

  void DrawCircle(const engine::math::Vector2f& /*center*/, float /*radius*/,
                  const engine::render::Color& /*color*/) override {}

  void DrawLine(const engine::math::Vector2f& /*start*/,
                const engine::math::Vector2f& /*end*/, float /*thickness*/,
                const engine::render::Color& /*color*/) override {}

  void DrawTexture(
      const engine::render::Texture2D& /*texture*/,
      const engine::render::SpriteDrawParams& /*params*/) override {}

  void DrawText(std::string_view /*text*/,
                const engine::math::Vector2f& /*position*/, float /*font_size*/,
                const engine::render::Color& /*color*/) override {}

  engine::math::Vector2f MeasureText(std::string_view text,
                                     float font_size) override {
    return {static_cast<float>(text.size()) * font_size * 0.5f, font_size};
  }

  std::shared_ptr<engine::render::Texture2D> LoadTextureFromFile(
      const std::string& /*path*/) override {
    return std::make_shared<FakeTexture>(engine::math::Vector2i{64, 64});
  }

  void LoadFont(const std::string& /*name*/,
                const std::string& /*path*/) override {}

  void SetFont(const std::string& /*name*/) override {}

  void Flush() override {}
};

class FakeRenderContext final : public engine::render::RenderContext {
 public:
  explicit FakeRenderContext(engine::render::Renderer2D& renderer)
      : renderer_(renderer) {}

  void BeginFrame() override {}
  void EndFrame() override {}
  void Clear(const engine::render::Color& /*color*/) override {}
  engine::render::Renderer2D& Get2DRenderer() override { return renderer_; }

 private:
  engine::render::Renderer2D& renderer_;
};

class FakeWindow final : public engine::render::Window {
 public:
  FakeWindow() : size_(1280, 720), context_(renderer_), input_manager_() {}

  void PollEvents() override {}
  bool ShouldClose() const override { return false; }
  void RequestClose() override {}

  void SetInputManager(
      std::shared_ptr<engine::input::InputManager> input_manager) override {
    input_manager_ = std::move(input_manager);
  }

  engine::math::Vector2i GetSize() const override { return size_; }
  void SetSize(const engine::math::Vector2i& size) override { size_ = size; }
  void SetTitle(std::string_view /*title*/) override {}
  void ToggleFullscreen() override {}
  float GetFrameTime() const override { return 0.0f; }
  engine::render::RenderContext& GetRenderContext() override {
    return context_;
  }

  engine::render::Renderer2D& Renderer() { return renderer_; }

 private:
  engine::math::Vector2i size_;
  FakeRenderer renderer_;
  FakeRenderContext context_;
  std::shared_ptr<engine::input::InputManager> input_manager_;
};

class FakeClientContext final : public client::ClientContext {
 public:
  FakeClientContext() : assets_(window_.Renderer()) {}

  engine::render::Renderer2D& Renderer() override { return window_.Renderer(); }

  engine::input::InputManager& Input() override { return input_; }

  const client::KeyBindings& KeyBindingSet() const override {
    return key_binding_service_.bindings();
  }

  const client::KeyBindingService& KeyBindingServiceRef() const override {
    return key_binding_service_;
  }

  client::KeyBindingUpdateResult UpdateKeyBinding(
      client::GameAction action, engine::input::Key key) override {
    return key_binding_service_.UpdateBinding(action, key);
  }

  engine::render::Window& Window() override { return window_; }

  std::shared_ptr<engine::audio::AudioEngine> Audio() override { return {}; }

  client::ClientAssetManager& Assets() override { return assets_; }

  client::ui::MenuBackground& MenuBackground() override {
    return menu_background_;
  }

  engine::util::Configuration& Config() override { return config_; }

  void OnPlay() override { ++play_calls_; }
  void OnOpenSettings() override { ++open_settings_calls_; }
  void OnCloseSettings() override { ++close_settings_calls_; }
  void OnQuitApplication() override { ++quit_calls_; }
  void OnQuitToMenu() override { ++quit_to_menu_calls_; }
  void OnGamePause() override { ++pause_calls_; }
  void OnGameResume() override { ++resume_calls_; }

  void SetConnectionConfig(std::string host, int port, std::string player_name,
                           std::string room_code,
                           std::string room_password = {}) override {
    last_host_ = std::move(host);
    last_port_ = port;
    last_player_ = std::move(player_name);
    last_room_ = std::move(room_code);
    last_password_ = std::move(room_password);
  }

  bool StartConnection() override {
    start_connection_calls_++;
    return true;
  }

  void RefreshRoomList(std::string host, std::uint16_t port) override {
    last_host_ = std::move(host);
    last_port_ = port;
  }

  void CreateRoom(std::string host, std::uint16_t port,
                  const std::string& room_name, bool is_private,
                  std::string room_password,
                  std::uint16_t max_players) override {
    last_host_ = std::move(host);
    last_port_ = port;
    last_room_name_ = room_name;
    last_private_ = is_private;
    last_password_ = std::move(room_password);
    last_max_players_ = max_players;
  }

  const std::vector<protocol::RoomSummary>& RoomDirectoryRooms()
      const override {
    return rooms_;
  }

  std::string RoomDirectoryStatus() const override { return room_status_; }

  std::optional<protocol::CreateRoomResponsePayload> ConsumeLastRoomCreation()
      override {
    return std::nullopt;
  }

  engine::ecs::Registry& World() override { return registry_; }
  const engine::ecs::Registry& World() const override { return registry_; }

  bool EnqueueCommand(const protocol::CommandPayload& /*payload*/) override {
    return true;
  }

  std::optional<std::uint32_t> CurrentWave() const override { return {}; }

  std::optional<float> LatestLatencyMs() const override { return {}; }

  std::optional<std::uint32_t> LocalPlayerId() const override { return {}; }

  std::string_view ConnectionStatus() const override {
    return connection_status_;
  }

  bool ConnectionActive() const override { return connection_active_; }

 private:
  FakeWindow window_;
  engine::input::InputManager input_;
  client::KeyBindingService key_binding_service_{};
  client::ClientAssetManager assets_;
  client::ui::MenuBackground menu_background_{""};
  engine::util::Configuration config_{};
  engine::ecs::Registry registry_{};
  std::vector<protocol::RoomSummary> rooms_{};
  std::string room_status_{"Lobby idle"};
  std::string connection_status_{"Connecting"};
  bool connection_active_{false};

  int play_calls_{0};
  int open_settings_calls_{0};
  int close_settings_calls_{0};
  int quit_calls_{0};
  int quit_to_menu_calls_{0};
  int pause_calls_{0};
  int resume_calls_{0};
  int start_connection_calls_{0};

  std::string last_host_{};
  int last_port_{0};
  std::string last_player_{};
  std::string last_room_{};
  std::string last_password_{};
  std::string last_room_name_{};
  bool last_private_{false};
  std::uint16_t last_max_players_{0};
};

}  // namespace

TEST(SceneManagerTest, RoutesMainMenuToLobby) {
  FakeClientContext context;
  client::SceneManager manager(context);

  manager.Initialize(client::ClientState::kMainMenu);

  EXPECT_EQ(manager.state(), client::ClientState::kMainMenu);
  ASSERT_NE(manager.CurrentScene(), nullptr);
  EXPECT_NE(dynamic_cast<client::MainMenuScene*>(manager.CurrentScene().get()),
            nullptr);

  manager.OnPlay();
  manager.CommitSceneChange();

  EXPECT_EQ(manager.state(), client::ClientState::kLobby);
  ASSERT_NE(manager.CurrentScene(), nullptr);
  EXPECT_NE(dynamic_cast<client::LobbyScene*>(manager.CurrentScene().get()),
            nullptr);
}

TEST(SceneManagerTest, ReturnsFromSettingsToPreviousState) {
  FakeClientContext context;
  client::SceneManager manager(context);

  manager.Initialize(client::ClientState::kLobby);

  manager.OnOpenSettings();
  EXPECT_EQ(manager.state(), client::ClientState::kSettings);
  ASSERT_TRUE(manager.settings_return_state().has_value());
  EXPECT_EQ(manager.settings_return_state().value(),
            client::ClientState::kLobby);
  manager.CommitSceneChange();
  ASSERT_NE(manager.CurrentScene(), nullptr);
  EXPECT_NE(
      dynamic_cast<client::OptionsMenuScene*>(manager.CurrentScene().get()),
      nullptr);

  manager.OnCloseSettings();
  manager.CommitSceneChange();

  EXPECT_EQ(manager.state(), client::ClientState::kLobby);
  EXPECT_FALSE(manager.settings_return_state().has_value());
  ASSERT_NE(manager.CurrentScene(), nullptr);
  EXPECT_NE(dynamic_cast<client::LobbyScene*>(manager.CurrentScene().get()),
            nullptr);
}

TEST(SceneManagerTest, RejectsInvalidTransition) {
  FakeClientContext context;
  client::SceneManager manager(context);

  manager.Initialize(client::ClientState::kMainMenu);

  const bool ok = manager.TransitionTo(client::ClientState::kInGame);

  EXPECT_FALSE(ok);
  EXPECT_EQ(manager.state(), client::ClientState::kMainMenu);
  ASSERT_NE(manager.CurrentScene(), nullptr);
  EXPECT_NE(dynamic_cast<client::MainMenuScene*>(manager.CurrentScene().get()),
            nullptr);
}
