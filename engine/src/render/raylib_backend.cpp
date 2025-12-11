#include "engine/render/raylib_backend.h"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "engine/input.h"
#include "engine/math/vector2.h"
#include "engine/render/renderer2d.h"
#include "engine/util/logging.h"

namespace engine::render {

namespace {

unsigned char ToByte(float value) {
  float clamped = std::clamp(value, 0.0f, 1.0f);
  return static_cast<unsigned char>(std::lround(clamped * 255.0f));
}

::Color ToRaylibColor(const Color& color) {
  return ::Color{ToByte(color.r), ToByte(color.g), ToByte(color.b),
                 ToByte(color.a)};
}

::Rectangle ToRaylibRect(const math::RectF& rect) {
  return ::Rectangle{rect.top_left_x_, rect.top_left_y_, rect.width_,
                     rect.height_};
}

::Vector2 ToRaylibVector(const math::Vector2f& vec) {
  return ::Vector2{vec.x, vec.y};
}

constexpr std::array<std::pair<input::Key, int>, 57> kKeyMappings{{
    {input::Key::kA, KEY_A},
    {input::Key::kB, KEY_B},
    {input::Key::kC, KEY_C},
    {input::Key::kD, KEY_D},
    {input::Key::kE, KEY_E},
    {input::Key::kF, KEY_F},
    {input::Key::kG, KEY_G},
    {input::Key::kH, KEY_H},
    {input::Key::kI, KEY_I},
    {input::Key::kJ, KEY_J},
    {input::Key::kK, KEY_K},
    {input::Key::kL, KEY_L},
    {input::Key::kM, KEY_M},
    {input::Key::kN, KEY_N},
    {input::Key::kO, KEY_O},
    {input::Key::kP, KEY_P},
    {input::Key::kQ, KEY_Q},
    {input::Key::kR, KEY_R},
    {input::Key::kS, KEY_S},
    {input::Key::kT, KEY_T},
    {input::Key::kU, KEY_U},
    {input::Key::kV, KEY_V},
    {input::Key::kW, KEY_W},
    {input::Key::kX, KEY_X},
    {input::Key::kY, KEY_Y},
    {input::Key::kZ, KEY_Z},
    {input::Key::kNum0, KEY_ZERO},
    {input::Key::kNum1, KEY_ONE},
    {input::Key::kNum2, KEY_TWO},
    {input::Key::kNum3, KEY_THREE},
    {input::Key::kNum4, KEY_FOUR},
    {input::Key::kNum5, KEY_FIVE},
    {input::Key::kNum6, KEY_SIX},
    {input::Key::kNum7, KEY_SEVEN},
    {input::Key::kNum8, KEY_EIGHT},
    {input::Key::kNum9, KEY_NINE},
    {input::Key::kPeriod, KEY_PERIOD},
    {input::Key::kComma, KEY_COMMA},
    {input::Key::kSlash, KEY_SLASH},
    {input::Key::kBackslash, KEY_BACKSLASH},
    {input::Key::kSemicolon, KEY_SEMICOLON},
    {input::Key::kEqual, KEY_EQUAL},
    {input::Key::kMinus, KEY_MINUS},
    {input::Key::kUp, KEY_UP},
    {input::Key::kDown, KEY_DOWN},
    {input::Key::kLeft, KEY_LEFT},
    {input::Key::kRight, KEY_RIGHT},
    {input::Key::kSpace, KEY_SPACE},
    {input::Key::kBackspace, KEY_BACKSPACE},
    {input::Key::kEnter, KEY_ENTER},
    {input::Key::kEscape, KEY_ESCAPE},
    {input::Key::kLeftShift, KEY_LEFT_SHIFT},
    {input::Key::kRightShift, KEY_RIGHT_SHIFT},
    {input::Key::kLeftControl, KEY_LEFT_CONTROL},
    {input::Key::kRightControl, KEY_RIGHT_CONTROL},
    {input::Key::kLeftAlt, KEY_LEFT_ALT},
    {input::Key::kRightAlt, KEY_RIGHT_ALT},
}};

// Raylib SIDE/EXTRA correspond to the typical back/forward buttons.
constexpr std::array<std::pair<input::MouseButton, int>, 5> kMouseMappings{{
    {input::MouseButton::kLeft, MOUSE_BUTTON_LEFT},
    {input::MouseButton::kRight, MOUSE_BUTTON_RIGHT},
    {input::MouseButton::kMiddle, MOUSE_BUTTON_MIDDLE},
    {input::MouseButton::kButton4, MOUSE_BUTTON_SIDE},
    {input::MouseButton::kButton5, MOUSE_BUTTON_EXTRA},
}};

class RaylibTexture2D final : public Texture2D {
 public:
  explicit RaylibTexture2D(::Texture2D texture) : texture_(texture) {}
  ~RaylibTexture2D() override {
    if (texture_.id != 0) {
      ::UnloadTexture(texture_);
    }
  }

  RaylibTexture2D(const RaylibTexture2D&) = delete;
  RaylibTexture2D& operator=(const RaylibTexture2D&) = delete;
  RaylibTexture2D(RaylibTexture2D&& other) noexcept : texture_(other.texture_) {
    other.texture_.id = 0;
  }

  RaylibTexture2D& operator=(RaylibTexture2D&& other) noexcept {
    if (this != &other) {
      if (texture_.id != 0) {
        ::UnloadTexture(texture_);
      }
      texture_ = other.texture_;
      other.texture_.id = 0;
    }
    return *this;
  }

  math::Vector2i GetSize() const override {
    return math::Vector2i(texture_.width, texture_.height);
  }

  const ::Texture2D& GetNative() const { return texture_; }

 private:
  ::Texture2D texture_{};
};

class RaylibRenderer2D final : public Renderer2D {
 public:
  void DrawRect(const math::RectF& rect, const Color& color) override {
    ::DrawRectangleRec(ToRaylibRect(rect), ToRaylibColor(color));
  }

  void DrawCircle(const math::Vector2f& center, float radius,
                  const Color& color) override {
    ::DrawCircleV(ToRaylibVector(center), radius, ToRaylibColor(color));
  }

  void DrawLine(const math::Vector2f& start, const math::Vector2f& end,
                float thickness, const Color& color) override {
    ::DrawLineEx(ToRaylibVector(start), ToRaylibVector(end), thickness,
                 ToRaylibColor(color));
  }

  void DrawTexture(const Texture2D& texture,
                   const SpriteDrawParams& params) override {
    const RaylibTexture2D& raylib_texture =
        [&texture]() -> const RaylibTexture2D& {
      try {
        return dynamic_cast<const RaylibTexture2D&>(texture);
      } catch (const std::bad_cast&) {
        throw std::runtime_error("Texture provided was not created by Raylib.");
      }
    }();

    const math::Vector2i texture_size = raylib_texture.GetSize();

    ::Rectangle source{
        0.0f,
        0.0f,
        static_cast<float>(texture_size.x),
        static_cast<float>(texture_size.y),
    };

    if (params.source.has_value()) {
      const auto& src = params.source.value();
      source = ::Rectangle{src.top_left_x_, src.top_left_y_, src.width_,
                           src.height_};
    }

    ::Rectangle dest{
        params.position.x,
        params.position.y,
        static_cast<float>(texture_size.x) * params.scale.x,
        static_cast<float>(texture_size.y) * params.scale.y,
    };

    ::Vector2 origin = ToRaylibVector(params.origin);
    ::DrawTexturePro(raylib_texture.GetNative(), source, dest, origin,
                     params.rotation, ToRaylibColor(params.tint));
  }

  void DrawText(std::string_view text, const math::Vector2f& position,
                float font_size, const Color& color) override {
    std::string text_copy(text);
    ::Font font =
        current_font_.texture.id == 0 ? ::GetFontDefault() : current_font_;
    ::DrawTextEx(font, text_copy.c_str(), ToRaylibVector(position), font_size,
                 1.0f, ToRaylibColor(color));
  }

  math::Vector2f MeasureText(std::string_view text, float font_size) override {
    std::string text_copy(text);
    ::Font font =
        current_font_.texture.id == 0 ? ::GetFontDefault() : current_font_;
    ::Vector2 size = ::MeasureTextEx(font, text_copy.c_str(), font_size, 1.0f);
    return {size.x, size.y};
  }

  std::shared_ptr<Texture2D> LoadTextureFromFile(
      const std::string& path) override {
    ::Texture2D texture = ::LoadTexture(path.c_str());
    if (texture.id == 0) {
      return nullptr;
    }
    return std::make_shared<RaylibTexture2D>(texture);
  }

  void LoadFont(const std::string& name, const std::string& path) override {
    ::Font font = ::LoadFont(path.c_str());
    if (font.texture.id != 0) {
      if (fonts_.count(name)) {
        ::UnloadFont(fonts_[name]);
      }
      fonts_[name] = font;
    } else {
      engine::util::Logger::Default().Error(
          "Failed to load font '", name, "' from path '", path, "'");
    }
  }

  void SetFont(const std::string& name) override {
    if (fonts_.count(name)) {
      current_font_ = fonts_[name];
    } else {
      engine::util::Logger::Default().Warn(
          "SetFont: Font '", name, "' not loaded");
    }
  }

  void Flush() override {}

  ~RaylibRenderer2D() override {
    for (auto& [name, font] : fonts_) {
      ::UnloadFont(font);
    }
    fonts_.clear();
  }

 private:
  std::unordered_map<std::string, ::Font> fonts_;
  ::Font current_font_ = {0};
};

class RaylibRenderContext final : public RenderContext {
 public:
  explicit RaylibRenderContext(RaylibRenderer2D& renderer)
      : renderer_(renderer) {}

  void BeginFrame() override { ::BeginDrawing(); }

  void EndFrame() override { ::EndDrawing(); }

  void Clear(const Color& color) override {
    ::ClearBackground(ToRaylibColor(color));
  }

  Renderer2D& Get2DRenderer() override { return renderer_; }

 private:
  RaylibRenderer2D& renderer_;
};

class RaylibWindow final : public Window {
 public:
  explicit RaylibWindow(const WindowConfig& config)
      : config_(config), context_(renderer_) {
    if (window_alive_) {
      throw std::runtime_error(
          "Only one Raylib window can exist at a time in this backend.");
    }

    unsigned int flags = 0;
    if (config.resizable) flags |= FLAG_WINDOW_RESIZABLE;
    if (config.vsync) flags |= FLAG_VSYNC_HINT;
    if (config.fullscreen) flags |= FLAG_FULLSCREEN_MODE;
    if (flags != 0) {
      ::SetConfigFlags(flags);
    }

    ::InitWindow(config.size.x, config.size.y, config.title.c_str());
    ::SetExitKey(0);

    input_manager_ = config.input_manager;
    if (input_manager_) {
      input_manager_->ClearState();
    }
    window_alive_ = true;

    if (config.target_fps > 0) {
      ::SetTargetFPS(config.target_fps);
    }
  }

  ~RaylibWindow() override {
    if (window_alive_) {
      ::CloseWindow();
      window_alive_ = false;
    }
  }

  void PollEvents() override {
    if (!input_manager_) {
      if (!input_manager_warning_logged_) {
        engine::util::Logger::Default().Error(
            "Input Manager is NULL in PollEvents");
        input_manager_warning_logged_ = true;
      }
      return;
    }

    ::Vector2 mouse_pos = ::GetMousePosition();
    input_manager_->SetMousePosition({mouse_pos.x, mouse_pos.y});

    for (const auto& [key, native_key] : kKeyMappings) {
      if (::IsKeyPressed(native_key)) {
        input_manager_->HandleKey(key, true);
      } else if (::IsKeyReleased(native_key)) {
        input_manager_->HandleKey(key, false);
      }
    }

    for (const auto& [button, native_button] : kMouseMappings) {
      if (::IsMouseButtonPressed(native_button)) {
        input_manager_->HandleMouseButton(button, true);
      } else if (::IsMouseButtonReleased(native_button)) {
        input_manager_->HandleMouseButton(button, false);
      }
    }
  }

  bool ShouldClose() const override {
    bool close = should_close_ || !window_alive_ || ::WindowShouldClose();
    if (close && !close_logged_) {
      engine::util::Logger::Default().Info("Window ShouldClose detected");
      close_logged_ = true;
    }
    return close;
  }

  void RequestClose() override { should_close_ = true; }

  void SetInputManager(
      std::shared_ptr<input::InputManager> input_manager) override {
    input_manager_ = input_manager;
    if (input_manager_) {
      input_manager_->ClearState();
    }
  }

  math::Vector2i GetSize() const override {
    return math::Vector2i(::GetScreenWidth(), ::GetScreenHeight());
  }

  void SetSize(const math::Vector2i& size) override {
    ::SetWindowSize(size.x, size.y);
  }

  void SetTitle(std::string_view title) override {
    std::string title_copy(title);
    ::SetWindowTitle(title_copy.c_str());
  }

  void ToggleFullscreen() override { ::ToggleFullscreen(); }

  float GetFrameTime() const override { return ::GetFrameTime(); }

  RenderContext& GetRenderContext() override { return context_; }

 private:
  WindowConfig config_;
  bool should_close_{false};
  mutable bool close_logged_{false};
  bool input_manager_warning_logged_{false};
  std::shared_ptr<input::InputManager> input_manager_{};
  RaylibRenderer2D renderer_;
  RaylibRenderContext context_;

  inline static bool window_alive_{false};
};

class RaylibBackend final : public WindowBackend {
 public:
  std::string_view Name() const override { return "raylib"; }

  std::unique_ptr<Window> CreateWindow(const WindowConfig& config) override {
    return std::make_unique<RaylibWindow>(config);
  }
};

}  // namespace

std::unique_ptr<WindowBackend> CreateRaylibBackend() {
  return std::make_unique<RaylibBackend>();
}

}  // namespace engine::render
