#include "client_app.h"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <system_error>

#include <raylib.h>

#include "engine/audio/raylib_audio_engine.h"
#include "engine/render/color.h"
#include "engine/render/raylib_backend.h"

namespace client {
namespace {

struct KeyMapping {
  engine::input::Key key;
  int raylib_key;
};

constexpr KeyMapping kKeyMappings[] = {
    {engine::input::Key::kA, KEY_A},
    {engine::input::Key::kB, KEY_B},
    {engine::input::Key::kC, KEY_C},
    {engine::input::Key::kD, KEY_D},
    {engine::input::Key::kE, KEY_E},
    {engine::input::Key::kF, KEY_F},
    {engine::input::Key::kG, KEY_G},
    {engine::input::Key::kH, KEY_H},
    {engine::input::Key::kI, KEY_I},
    {engine::input::Key::kJ, KEY_J},
    {engine::input::Key::kK, KEY_K},
    {engine::input::Key::kL, KEY_L},
    {engine::input::Key::kM, KEY_M},
    {engine::input::Key::kN, KEY_N},
    {engine::input::Key::kO, KEY_O},
    {engine::input::Key::kP, KEY_P},
    {engine::input::Key::kQ, KEY_Q},
    {engine::input::Key::kR, KEY_R},
    {engine::input::Key::kS, KEY_S},
    {engine::input::Key::kT, KEY_T},
    {engine::input::Key::kU, KEY_U},
    {engine::input::Key::kV, KEY_V},
    {engine::input::Key::kW, KEY_W},
    {engine::input::Key::kX, KEY_X},
    {engine::input::Key::kY, KEY_Y},
    {engine::input::Key::kZ, KEY_Z},
    {engine::input::Key::kNum0, KEY_ZERO},
    {engine::input::Key::kNum1, KEY_ONE},
    {engine::input::Key::kNum2, KEY_TWO},
    {engine::input::Key::kNum3, KEY_THREE},
    {engine::input::Key::kNum4, KEY_FOUR},
    {engine::input::Key::kNum5, KEY_FIVE},
    {engine::input::Key::kNum6, KEY_SIX},
    {engine::input::Key::kNum7, KEY_SEVEN},
    {engine::input::Key::kNum8, KEY_EIGHT},
    {engine::input::Key::kNum9, KEY_NINE},
    {engine::input::Key::kUp, KEY_UP},
    {engine::input::Key::kDown, KEY_DOWN},
    {engine::input::Key::kLeft, KEY_LEFT},
    {engine::input::Key::kRight, KEY_RIGHT},
    {engine::input::Key::kSpace, KEY_SPACE},
    {engine::input::Key::kEnter, KEY_ENTER},
    {engine::input::Key::kEscape, KEY_ESCAPE},
    {engine::input::Key::kLeftShift, KEY_LEFT_SHIFT},
    {engine::input::Key::kRightShift, KEY_RIGHT_SHIFT},
    {engine::input::Key::kLeftControl, KEY_LEFT_CONTROL},
    {engine::input::Key::kRightControl, KEY_RIGHT_CONTROL},
    {engine::input::Key::kLeftAlt, KEY_LEFT_ALT},
    {engine::input::Key::kRightAlt, KEY_RIGHT_ALT},
};

struct MouseMapping {
  engine::input::MouseButton button;
  int raylib_button;
};

constexpr MouseMapping kMouseMappings[] = {
    {engine::input::MouseButton::kLeft, MOUSE_BUTTON_LEFT},
    {engine::input::MouseButton::kRight, MOUSE_BUTTON_RIGHT},
    {engine::input::MouseButton::kMiddle, MOUSE_BUTTON_MIDDLE},
    {engine::input::MouseButton::kButton4, MOUSE_BUTTON_SIDE},
    {engine::input::MouseButton::kButton5, MOUSE_BUTTON_EXTRA},
};

}  // namespace

ClientApp::ClientApp() = default;

ClientApp::~ClientApp() = default;

int ClientApp::Run() {
  if (!Initialize()) return EXIT_FAILURE;
  if (!loop_) return EXIT_FAILURE;
  loop_->run([this](engine::time::TimeDelta dt) { return Tick(dt); });
  return EXIT_SUCCESS;
}

bool ClientApp::Initialize() {
  window_config_.title = "R-Type Client";
  render_backend_ = engine::render::CreateRaylibBackend();
  if (render_backend_) {
    window_ = render_backend_->CreateWindow(window_config_);
  }
  if (!window_) {
    std::cerr << "Failed to create render window" << std::endl;
    return false;
  }

  audio_engine_ = engine::audio::CreateRaylibAudioEngine();

  std::error_code bind_error =
      net_socket_.bind(engine::net::Endpoint::AnyIpv4(0));
  if (bind_error) {
    std::cerr << "UDP bind failed: " << bind_error.message() << std::endl;
  }

  input_manager_.BindKey("Quit", engine::input::Key::kEscape);
  loop_ = std::make_unique<engine::time::VariableTimestepLoop>(
      static_cast<float>(window_config_.target_fps));
  return true;
}

bool ClientApp::Tick(engine::time::TimeDelta dt) {
  if (should_exit_) return false;
  if (!window_) return false;

  window_->PollEvents();
  PumpInput();
  ServiceNetwork();
  registry_.UpdateSystems(dt);
  RenderFrame();
  UpdateAudio();

  return !should_exit_ && !window_->ShouldClose();
}

void ClientApp::PumpInput() {
  for (const auto& mapping : kKeyMappings) {
    const bool pressed = ::IsKeyDown(mapping.raylib_key);
    input_manager_.HandleKey(mapping.key, pressed);
  }

  for (const auto& mapping : kMouseMappings) {
    const bool pressed = ::IsMouseButtonDown(mapping.raylib_button);
    input_manager_.HandleMouseButton(mapping.button, pressed);
  }

  for (const auto& event : input_manager_.ConsumeEvents()) {
    if (event.action == "Quit" &&
        event.type == engine::input::ActionEventType::kPressed) {
      should_exit_ = true;
    }
  }
}

void ClientApp::ServiceNetwork() {
  std::array<std::byte, 2048> buffer{};
  auto result = net_socket_.receive_from(buffer.data(), buffer.size());

  if (result.error) {
    if (result.error == asio::error::would_block ||
        result.error == asio::error::try_again) {
      return;
    }
    std::cerr << "Network receive error: " << result.error.message()
              << std::endl;
    return;
  }
}

void ClientApp::RenderFrame() {
  if (!window_) return;

  engine::render::RenderContext& context = window_->GetRenderContext();
  context.BeginFrame();
  context.Clear(engine::render::Color::Black());
  context.EndFrame();
}

void ClientApp::UpdateAudio() {
  if (audio_engine_) {
    audio_engine_->Update();
  }
}

}  // namespace client
