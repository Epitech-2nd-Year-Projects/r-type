#include "engine/render/raylib_backend.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGDI
#define NOGDI
#endif
#endif
#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "engine/math/vector2.h"
#include "engine/render/renderer2d.h"

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
    const auto* raylib_texture = dynamic_cast<const RaylibTexture2D*>(&texture);
    if (raylib_texture == nullptr) {
      throw std::runtime_error("Texture provided was not created by Raylib.");
    }

    const math::Vector2i texture_size = raylib_texture->GetSize();

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
    ::DrawTexturePro(raylib_texture->GetNative(), source, dest, origin,
                     params.rotation, ToRaylibColor(params.tint));
  }

  void DrawText(std::string_view text, const math::Vector2f& position,
                float font_size, const Color& color) override {
    std::string text_copy(text);
    ::DrawTextEx(::GetFontDefault(), text_copy.c_str(),
                 ToRaylibVector(position), font_size, 1.0f,
                 ToRaylibColor(color));
  }

  std::shared_ptr<Texture2D> LoadTextureFromFile(
      const std::string& path) override {
    ::Texture2D texture = ::LoadTexture(path.c_str());
    if (texture.id == 0) {
      return nullptr;
    }
    return std::make_shared<RaylibTexture2D>(texture);
  }

  void Flush() override {}
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

  void PollEvents() override { ::PollInputEvents(); }

  bool ShouldClose() const override {
    return should_close_ || !window_alive_ || ::WindowShouldClose();
  }

  void RequestClose() override { should_close_ = true; }

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

  float GetFrameTime() const override { return ::GetFrameTime(); }

  RenderContext& GetRenderContext() override { return context_; }

 private:
  WindowConfig config_;
  bool should_close_{false};
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
