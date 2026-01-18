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
#include "engine/math/vector3.h"
#include "engine/render/animation.h"
#include "engine/render/camera3d.h"
#include "engine/render/model.h"
#include "engine/render/renderer2d.h"
#include "engine/render/renderer3d.h"
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

::Vector3 ToRaylibVector3(const math::Vector3f& vec) {
  return ::Vector3{vec.x, vec.y, vec.z};
}

// Map physical QWERTY scancodes to AZERTY logical keys to keep gameplay and
// text handling aligned with the AZERTY layout irrespective of OS settings.
constexpr std::array<std::pair<input::Key, int>, 61> kKeyMappings{{
    {input::Key::kA, KEY_Q},
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
    {input::Key::kM, KEY_SEMICOLON},
    {input::Key::kN, KEY_N},
    {input::Key::kO, KEY_O},
    {input::Key::kP, KEY_P},
    {input::Key::kQ, KEY_A},
    {input::Key::kR, KEY_R},
    {input::Key::kS, KEY_S},
    {input::Key::kT, KEY_T},
    {input::Key::kU, KEY_U},
    {input::Key::kV, KEY_V},
    {input::Key::kW, KEY_Z},
    {input::Key::kX, KEY_X},
    {input::Key::kY, KEY_Y},
    {input::Key::kZ, KEY_W},
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
    {input::Key::kComma, KEY_M},
    {input::Key::kSlash, KEY_SLASH},
    {input::Key::kBackslash, KEY_BACKSLASH},
    {input::Key::kSemicolon, KEY_COMMA},
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
    {input::Key::kF1, KEY_F1},
    {input::Key::kF2, KEY_F2},
    {input::Key::kF3, KEY_F3},
    {input::Key::kF4, KEY_F4},
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
      if (::IsWindowReady()) {
        ::UnloadTexture(texture_);
      }
      texture_.id = 0;
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

  void DrawRing(const math::Vector2f& center, float inner_radius,
                float outer_radius, float start_angle, float end_angle,
                int segments, const Color& color) override {
    ::DrawRing(ToRaylibVector(center), inner_radius, outer_radius, start_angle,
               end_angle, segments, ToRaylibColor(color));
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
    constexpr int kDefaultFontBaseSize = 64;
    ::Font font = ::LoadFontEx(path.c_str(), kDefaultFontBaseSize, nullptr, 0);
    if (font.texture.id != 0) {
      ::SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
      if (fonts_.count(name)) {
        ::UnloadFont(fonts_[name]);
      }
      fonts_[name] = font;
    } else {
      engine::util::Logger::Default().Error("Failed to load font '", name,
                                            "' from path '", path, "'");
    }
  }

  void SetFont(const std::string& name) override {
    if (fonts_.count(name)) {
      current_font_ = fonts_[name];
    } else {
      engine::util::Logger::Default().Warn("SetFont: Font '", name,
                                           "' not loaded");
    }
  }

  void Flush() override {}

  void ReleaseFonts() {
    if (!::IsWindowReady()) {
      fonts_.clear();
      return;
    }
    for (auto& [name, font] : fonts_) {
      ::UnloadFont(font);
    }
    fonts_.clear();
  }

  ~RaylibRenderer2D() override { ReleaseFonts(); }

 private:
  std::unordered_map<std::string, ::Font> fonts_;
  ::Font current_font_ = {0};
};

// ============================================================================
// 3D Rendering
// ============================================================================

class RaylibModel final : public Model {
 public:
  explicit RaylibModel(::Model model) : model_(model) {
    bounding_box_ = ::GetModelBoundingBox(model_);
  }

  ~RaylibModel() override {
    if (model_.meshCount > 0 && ::IsWindowReady()) {
      ::UnloadModel(model_);
    }
  }

  RaylibModel(const RaylibModel&) = delete;
  RaylibModel& operator=(const RaylibModel&) = delete;

  math::Vector3f GetBoundingBoxSize() const override {
    return math::Vector3f(bounding_box_.max.x - bounding_box_.min.x,
                          bounding_box_.max.y - bounding_box_.min.y,
                          bounding_box_.max.z - bounding_box_.min.z);
  }

  math::Vector3f GetBoundingBoxCenter() const override {
    return math::Vector3f((bounding_box_.max.x + bounding_box_.min.x) / 2.0f,
                          (bounding_box_.max.y + bounding_box_.min.y) / 2.0f,
                          (bounding_box_.max.z + bounding_box_.min.z) / 2.0f);
  }

  const ::Model& GetNative() const { return model_; }
  ::Model& GetNativeMutable() { return model_; }

 private:
  ::Model model_{};
  ::BoundingBox bounding_box_{};
};

class RaylibAnimation final : public Animation {
 public:
  explicit RaylibAnimation(const ::ModelAnimation& anim) : animation_(anim) {}

  std::uint32_t GetFrameCount() const override {
    return static_cast<std::uint32_t>(animation_.frameCount);
  }

  std::string_view GetName() const override { return animation_.name; }

  std::uint32_t GetBoneCount() const override {
    return static_cast<std::uint32_t>(animation_.boneCount);
  }

  const ::ModelAnimation& GetNative() const { return animation_; }

 private:
  ::ModelAnimation animation_;
};

class RaylibAnimationSet final : public AnimationSet {
 public:
  RaylibAnimationSet(::ModelAnimation* anims, int count)
      : native_animations_(anims), count_(count) {
    animations_.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
      animations_.push_back(std::make_unique<RaylibAnimation>(anims[i]));
    }
  }

  ~RaylibAnimationSet() override {
    if (native_animations_ && count_ > 0 && ::IsWindowReady()) {
      ::UnloadModelAnimations(native_animations_, count_);
    }
  }

  RaylibAnimationSet(const RaylibAnimationSet&) = delete;
  RaylibAnimationSet& operator=(const RaylibAnimationSet&) = delete;

  std::size_t GetCount() const override { return animations_.size(); }

  const Animation* GetAnimation(std::size_t index) const override {
    return index < animations_.size() ? animations_[index].get() : nullptr;
  }

  const Animation* FindByName(std::string_view name) const override {
    for (const auto& anim : animations_) {
      if (anim->GetName() == name) return anim.get();
    }
    return nullptr;
  }

 private:
  ::ModelAnimation* native_animations_{nullptr};
  int count_{0};
  std::vector<std::unique_ptr<RaylibAnimation>> animations_;
};

class RaylibRenderer3D final : public Renderer3D {
 public:
  void Begin3D(const Camera3D& camera) override {
    ::Camera3D raylib_camera{};
    raylib_camera.position = ToRaylibVector3(camera.GetPosition());
    raylib_camera.target = ToRaylibVector3(camera.GetTarget());
    raylib_camera.up = ToRaylibVector3(camera.GetUp());
    raylib_camera.fovy = camera.GetFov();
    raylib_camera.projection = static_cast<int>(camera.GetProjection());
    ::BeginMode3D(raylib_camera);
  }

  void End3D() override { ::EndMode3D(); }

  void DrawCube(const math::Vector3f& position, const math::Vector3f& size,
                const Color& color) override {
    ::DrawCubeV(ToRaylibVector3(position), ToRaylibVector3(size),
                ToRaylibColor(color));
  }

  void DrawCubeWires(const math::Vector3f& position, const math::Vector3f& size,
                     const Color& color) override {
    ::DrawCubeWiresV(ToRaylibVector3(position), ToRaylibVector3(size),
                     ToRaylibColor(color));
  }

  void DrawSphere(const math::Vector3f& center, float radius,
                  const Color& color) override {
    ::DrawSphere(ToRaylibVector3(center), radius, ToRaylibColor(color));
  }

  void DrawSphereWires(const math::Vector3f& center, float radius, int rings,
                       int slices, const Color& color) override {
    ::DrawSphereWires(ToRaylibVector3(center), radius, rings, slices,
                      ToRaylibColor(color));
  }

  void DrawCylinder(const math::Vector3f& position, float radius_top,
                    float radius_bottom, float height, int slices,
                    const Color& color) override {
    ::DrawCylinder(ToRaylibVector3(position), radius_top, radius_bottom, height,
                   slices, ToRaylibColor(color));
  }

  void DrawCylinderWires(const math::Vector3f& position, float radius_top,
                         float radius_bottom, float height, int slices,
                         const Color& color) override {
    ::DrawCylinderWires(ToRaylibVector3(position), radius_top, radius_bottom,
                        height, slices, ToRaylibColor(color));
  }

  void DrawPlane(const math::Vector3f& center, const math::Vector2f& size,
                 const Color& color) override {
    ::DrawPlane(ToRaylibVector3(center), ToRaylibVector(size),
                ToRaylibColor(color));
  }

  void DrawGrid(int slices, float spacing) override {
    ::DrawGrid(slices, spacing);
  }

  void DrawLine3D(const math::Vector3f& start, const math::Vector3f& end,
                  const Color& color) override {
    ::DrawLine3D(ToRaylibVector3(start), ToRaylibVector3(end),
                 ToRaylibColor(color));
  }

  void DrawPoint3D(const math::Vector3f& position,
                   const Color& color) override {
    ::DrawPoint3D(ToRaylibVector3(position), ToRaylibColor(color));
  }

  void DrawModel(const Model& model, const ModelDrawParams& params) override {
    const auto& raylib_model = dynamic_cast<const RaylibModel&>(model);
    ::DrawModelEx(raylib_model.GetNative(), ToRaylibVector3(params.position),
                  ToRaylibVector3(params.rotation_axis), params.rotation_angle,
                  ToRaylibVector3(params.scale), ToRaylibColor(params.tint));
  }

  void DrawModelWires(const Model& model,
                      const ModelDrawParams& params) override {
    const auto& raylib_model = dynamic_cast<const RaylibModel&>(model);
    ::DrawModelWiresEx(
        raylib_model.GetNative(), ToRaylibVector3(params.position),
        ToRaylibVector3(params.rotation_axis), params.rotation_angle,
        ToRaylibVector3(params.scale), ToRaylibColor(params.tint));
  }

  std::shared_ptr<Model> LoadModelFromFile(const std::string& path) override {
    ::Model model = ::LoadModel(path.c_str());
    if (model.meshCount == 0) {
      engine::util::Logger::Default().Error("Failed to load model from '", path,
                                            "'");
      return nullptr;
    }
    return std::make_shared<RaylibModel>(model);
  }

  bool SetModelTexture(Model& model, const std::string& texture_path) override {
    auto& raylib_model = dynamic_cast<RaylibModel&>(model);
    ::Texture2D texture = ::LoadTexture(texture_path.c_str());
    if (texture.id == 0) {
      engine::util::Logger::Default().Error("Failed to load texture from '",
                                            texture_path, "'");
      return false;
    }

    ::Model& native_model = raylib_model.GetNativeMutable();
    for (int i = 0; i < native_model.materialCount; ++i) {
      native_model.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    }

    engine::util::Logger::Default().Info("Applied texture '", texture_path,
                                         "' to model");
    return true;
  }

  void SetLighting(const LightingConfig& config) override {
    // Basic lighting implementation stores the config for future shader-based
    // lighting. For now, Raylib's default lighting is used.
    lighting_enabled_ = true;
    lighting_config_ = config;
    // Note: Full lighting implementation would require loading and setting up
    // custom shaders with uniforms for ambient and directional lights.
  }

  void DisableLighting() override { lighting_enabled_ = false; }

  std::shared_ptr<AnimationSet> LoadAnimationsFromFile(
      const std::string& path) override {
    int anim_count = 0;
    ::ModelAnimation* anims = ::LoadModelAnimations(path.c_str(), &anim_count);
    if (!anims || anim_count == 0) {
      engine::util::Logger::Default().Error("Failed to load animations from '",
                                            path, "'");
      return nullptr;
    }
    engine::util::Logger::Default().Info("Loaded ", anim_count,
                                         " animations from '", path, "'");
    return std::make_shared<RaylibAnimationSet>(anims, anim_count);
  }

  void UpdateModelAnimation(Model& model, const Animation& animation,
                            std::uint32_t frame) override {
    auto& raylib_model = dynamic_cast<RaylibModel&>(model);
    const auto& raylib_anim = dynamic_cast<const RaylibAnimation&>(animation);

    const int safe_frame =
        static_cast<int>(std::min(frame, animation.GetFrameCount() - 1));

    ::UpdateModelAnimation(raylib_model.GetNativeMutable(),
                           raylib_anim.GetNative(), safe_frame);
  }

 private:
  bool lighting_enabled_{false};
  LightingConfig lighting_config_;
};

class RaylibRenderContext final : public RenderContext {
 public:
  RaylibRenderContext(RaylibRenderer2D& renderer2d,
                      RaylibRenderer3D& renderer3d)
      : renderer2d_(renderer2d), renderer3d_(renderer3d) {}

  void BeginFrame() override { ::BeginDrawing(); }

  void EndFrame() override { ::EndDrawing(); }

  void Clear(const Color& color) override {
    ::ClearBackground(ToRaylibColor(color));
  }

  Renderer2D& Get2DRenderer() override { return renderer2d_; }

  Renderer3D& Get3DRenderer() override { return renderer3d_; }

 private:
  RaylibRenderer2D& renderer2d_;
  RaylibRenderer3D& renderer3d_;
};

class RaylibWindow final : public Window {
 public:
  explicit RaylibWindow(const WindowConfig& config)
      : config_(config), context_(renderer2d_, renderer3d_) {
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
      renderer2d_.ReleaseFonts();
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
    input_manager_->SetMouseWheelDelta(::GetMouseWheelMove());

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

  void SetVsync(bool enabled) override {
    if (enabled) {
      ::SetWindowState(FLAG_VSYNC_HINT);
    } else {
      ::ClearWindowState(FLAG_VSYNC_HINT);
    }
  }

  void SetTargetFps(int target_fps) override { ::SetTargetFPS(target_fps); }

  float GetFrameTime() const override { return ::GetFrameTime(); }

  RenderContext& GetRenderContext() override { return context_; }

 private:
  WindowConfig config_;
  bool should_close_{false};
  mutable bool close_logged_{false};
  bool input_manager_warning_logged_{false};
  std::shared_ptr<input::InputManager> input_manager_{};
  RaylibRenderer2D renderer2d_;
  RaylibRenderer3D renderer3d_;
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
