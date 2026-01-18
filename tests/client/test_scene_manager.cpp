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
#include "engine/render/renderer3d.h"
#include "engine/render/window.h"
#include "engine/util/config.h"
#include "input/key_binding_service.h"
#include "lobby_chat_service.h"
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

  void DrawRing(const engine::math::Vector2f& /*center*/, float /*inner_radius*/,
                float /*outer_radius*/, float /*start_angle*/,
                float /*end_angle*/, int /*segments*/,
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

class FakeRenderer3D final : public engine::render::Renderer3D {
 public:
  void Begin3D(const engine::render::Camera3D& /*camera*/) override {}
  void End3D() override {}
  void DrawCube(const engine::math::Vector3f& /*position*/,
                const engine::math::Vector3f& /*size*/,
                const engine::render::Color& /*color*/) override {}
  void DrawCubeWires(const engine::math::Vector3f& /*position*/,
                     const engine::math::Vector3f& /*size*/,
                     const engine::render::Color& /*color*/) override {}
  void DrawSphere(const engine::math::Vector3f& /*center*/, float /*radius*/,
                  const engine::render::Color& /*color*/) override {}
  void DrawSphereWires(const engine::math::Vector3f& /*center*/,
                       float /*radius*/, int /*rings*/, int /*slices*/,
                       const engine::render::Color& /*color*/) override {}
  void DrawCylinder(const engine::math::Vector3f& /*position*/,
                    float /*radius_top*/, float /*radius_bottom*/,
                    float /*height*/, int /*slices*/,
                    const engine::render::Color& /*color*/) override {}
  void DrawCylinderWires(const engine::math::Vector3f& /*position*/,
                         float /*radius_top*/, float /*radius_bottom*/,
                         float /*height*/, int /*slices*/,
                         const engine::render::Color& /*color*/) override {}
  void DrawPlane(const engine::math::Vector3f& /*center*/,
                 const engine::math::Vector2f& /*size*/,
                 const engine::render::Color& /*color*/) override {}
  void DrawGrid(int /*slices*/, float /*spacing*/) override {}
  void DrawLine3D(const engine::math::Vector3f& /*start*/,
                  const engine::math::Vector3f& /*end*/,
                  const engine::render::Color& /*color*/) override {}
  void DrawPoint3D(const engine::math::Vector3f& /*position*/,
                   const engine::render::Color& /*color*/) override {}
  void DrawModel(const engine::render::Model& /*model*/,
                 const engine::render::ModelDrawParams& /*params*/) override {}
  void DrawModelWires(
      const engine::render::Model& /*model*/,
      const engine::render::ModelDrawParams& /*params*/) override {}
  std::shared_ptr<engine::render::Model> LoadModelFromFile(
      const std::string& /*path*/) override {
    return nullptr;
  }
  void SetLighting(const engine::render::LightingConfig& /*config*/) override {}
  void DisableLighting() override {}
  bool SetModelTexture(engine::render::Model& /*model*/,
                       const std::string& /*texture_path*/) override {
    return false;
  }
  std::shared_ptr<engine::render::AnimationSet> LoadAnimationsFromFile(
      const std::string& /*path*/) override {
    return nullptr;
  }
  void UpdateModelAnimation(engine::render::Model& /*model*/,
                            const engine::render::Animation& /*animation*/,
                            uint32_t /*frame*/) override {}
};

class FakeRenderContext final : public engine::render::RenderContext {
 public:
  explicit FakeRenderContext(engine::render::Renderer2D& renderer2d,
                             engine::render::Renderer3D& renderer3d)
      : renderer2d_(renderer2d), renderer3d_(renderer3d) {}

  void BeginFrame() override {}
  void EndFrame() override {}
  void Clear(const engine::render::Color& /*color*/) override {}
  engine::render::Renderer2D& Get2DRenderer() override { return renderer2d_; }
  engine::render::Renderer3D& Get3DRenderer() override { return renderer3d_; }

 private:
  engine::render::Renderer2D& renderer2d_;
  engine::render::Renderer3D& renderer3d_;
};

class FakeWindow final : public engine::render::Window {
 public:
  FakeWindow()
      : size_(1280, 720),
        context_(renderer2d_, renderer3d_),
        input_manager_() {}

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
  void SetVsync(bool enabled) override { vsync_ = enabled; }
  void SetTargetFps(int target_fps) override { target_fps_ = target_fps; }
  float GetFrameTime() const override { return 0.0f; }
  engine::render::RenderContext& GetRenderContext() override {
    return context_;
  }

  engine::render::Renderer2D& Renderer() { return renderer2d_; }

 private:
  engine::math::Vector2i size_;
  FakeRenderer renderer2d_;
  FakeRenderer3D renderer3d_;
  FakeRenderContext context_;
  std::shared_ptr<engine::input::InputManager> input_manager_;
  bool vsync_{false};
  int target_fps_{0};
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

  engine::math::Vector2i RenderSize() const override { return render_size_; }

  std::shared_ptr<engine::audio::AudioEngine> Audio() override { return {}; }

  void SetAudioVolumes(float master_volume, float music_volume,
                       float sfx_volume) override {
    last_master_volume_ = master_volume;
    last_music_volume_ = music_volume;
    last_sfx_volume_ = sfx_volume;
  }

  void SetVideoSettings(int resolution_width, int resolution_height,
                        bool fullscreen, bool vsync, int target_fps) override {
    last_resolution_width_ = resolution_width;
    last_resolution_height_ = resolution_height;
    render_size_ = {resolution_width, resolution_height};
    last_fullscreen_ = fullscreen;
    last_vsync_ = vsync;
    last_target_fps_ = target_fps;
  }

  client::ClientAssetManager& Assets() override { return assets_; }

  client::ui::MenuBackground& MenuBackground() override {
    return menu_background_;
  }

  engine::util::Configuration& Config() override { return config_; }

  void OnPlay() override { ++play_calls_; }
  void OnOpenSettings() override { ++open_settings_calls_; }
  void OnOpenAudioSettings() override { ++open_audio_settings_calls_; }
  void OnOpenVideoSettings() override { ++open_video_settings_calls_; }
  void OnCloseAudioSettings() override { ++close_audio_settings_calls_; }
  void OnCloseSettings() override { ++close_settings_calls_; }
  void OnCloseVideoSettings() override { ++close_video_settings_calls_; }
  void OnOpenProfile() override { ++open_profile_calls_; }
  void OnCloseProfile() override { ++close_profile_calls_; }
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
                  std::string room_password, std::uint16_t max_players,
                  protocol::Difficulty difficulty) override {
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

  bool EnqueueGameplayPing(
      const protocol::GameplayPingPayload& /*payload*/) override {
    return true;
  }

  std::optional<std::uint32_t> CurrentWave() const override { return {}; }

  std::optional<float> LatestLatencyMs() const override { return {}; }

  std::optional<std::uint32_t> LocalPlayerId() const override { return {}; }

  std::string_view ConnectionStatus() const override {
    return connection_status_;
  }

  bool ConnectionActive() const override { return connection_active_; }

  client::PlayerProfile& Profile() override { return profile_; }
  const client::PlayerProfile& Profile() const override { return profile_; }
  void SaveProfile() override { ++save_profile_calls_; }

  client::LobbyChatService& ChatService() override { return chat_service_; }

 private:
  FakeWindow window_;
  engine::math::Vector2i render_size_{1280, 720};
  engine::input::InputManager input_;
  client::KeyBindingService key_binding_service_{};
  client::ClientAssetManager assets_;
  client::ui::MenuBackground menu_background_{""};
  engine::util::Configuration config_{};
  client::PlayerProfile profile_{};
  client::LobbyChatService chat_service_{[](const protocol::CommandPayload&) {
    return true;
  }};
  engine::ecs::Registry registry_{};
  std::vector<protocol::RoomSummary> rooms_{};
  std::string room_status_{"Lobby idle"};
  std::string connection_status_{"Connecting"};
  bool connection_active_{false};
  float last_master_volume_{0.0f};
  float last_music_volume_{0.0f};
  float last_sfx_volume_{0.0f};
  int last_resolution_width_{0};
  int last_resolution_height_{0};
  bool last_fullscreen_{false};
  bool last_vsync_{false};
  int last_target_fps_{0};

  int play_calls_{0};
  int open_settings_calls_{0};
  int open_audio_settings_calls_{0};
  int open_video_settings_calls_{0};
  int close_audio_settings_calls_{0};
  int close_settings_calls_{0};
  int close_video_settings_calls_{0};
  int open_profile_calls_{0};
  int close_profile_calls_{0};
  int save_profile_calls_{0};
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
