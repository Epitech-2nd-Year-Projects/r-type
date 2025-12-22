#include "engine/app/engine_runtime.h"

#include <memory>
#include <stdexcept>
#include <utility>

#include "engine/audio/raylib_audio_engine.h"
#include "engine/profiler/profiler.h"
#include "engine/profiler/profiler_overlay.h"
#include "engine/render.h"
#include "engine/render/raylib_backend.h"

namespace engine::app {

namespace {

std::unique_ptr<render::WindowBackend> DefaultWindowBackend() {
  return render::CreateRaylibBackend();
}

}  // namespace

EngineRuntime::EngineRuntime()
    : logger_(util::Logger::Default()),
      input_(std::make_shared<input::InputManager>()) {}

EngineRuntime::~EngineRuntime() = default;

std::unique_ptr<EngineRuntime> EngineRuntime::Create(
    const EngineRuntimeConfig& config) {
  auto runtime = std::unique_ptr<EngineRuntime>(new EngineRuntime());
  try {
    runtime->Initialize(config);
  } catch (const std::exception& ex) {
    util::Logger::Default().Error("Engine initialization failed: ", ex.what());
    return nullptr;
  }
  return runtime;
}

util::Configuration& EngineRuntime::Config() { return config_; }

util::Logger& EngineRuntime::Logger() { return logger_.get(); }

input::InputManager& EngineRuntime::Input() { return *input_; }

render::Window& EngineRuntime::Window() { return *window_; }

render::RenderContext& EngineRuntime::RenderContext() {
  return window_->GetRenderContext();
}

render::Renderer2D& EngineRuntime::Renderer() {
  return window_->GetRenderContext().Get2DRenderer();
}

std::shared_ptr<audio::AudioEngine> EngineRuntime::Audio() { return audio_; }

profiler::Profiler& EngineRuntime::Profiler() {
  return profiler::Profiler::Get();
}

profiler::ProfilerOverlay& EngineRuntime::ProfilerOverlay() {
  return *profiler_overlay_;
}

bool EngineRuntime::Pump() {
  engine::profiler::ScopedTimer frame_timer("Engine::Pump");
  if (window_) {
    window_->PollEvents();
    if (window_->ShouldClose()) {
      return false;
    }
  }

  if (audio_) {
    audio_->Update();
  }

  return true;
}

void EngineRuntime::Initialize(const EngineRuntimeConfig& config) {
  logger_ = util::Logger::Default();
  logger_.get().SetLevel(config.log_level);

  if (!config.colorize_logs) {
    util::Logger::ClearDefaultSinks();
    util::Logger::AddDefaultSink(std::make_shared<util::ConsoleSink>(false));
  }

  auto backend_factory = config.window_backend_factory;
  if (!backend_factory) {
    backend_factory = DefaultWindowBackend;
  }

  window_backend_ = backend_factory();
  if (!window_backend_) {
    throw std::runtime_error("Window backend creation failed");
  }

  render::WindowConfig window_config = config.window_config;
  window_config.input_manager = input_;
  window_ = window_backend_->CreateWindow(window_config);
  if (!window_) {
    throw std::runtime_error("Window creation failed");
  }

  if (config.enable_audio) {
    audio_ = audio::CreateRaylibAudioEngine();
  }

  profiler_overlay_ = std::make_unique<profiler::ProfilerOverlay>();

  logger_.get().Info("Engine ready using backend ", window_backend_->Name());
}

}  // namespace engine::app
