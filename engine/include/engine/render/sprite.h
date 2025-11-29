#ifndef ENGINE_RENDER_SPRITE_H_
#define ENGINE_RENDER_SPRITE_H_

#include <memory>
#include <optional>
#include <vector>

#include "engine/math/rect.h"
#include "engine/math/vector2.h"
#include "engine/render/color.h"
#include "engine/render/renderer2d.h"

namespace engine::render {

/**
 * @brief Convenience wrapper around a 2D texture and draw parameters.
 */
class Sprite {
 public:
  Sprite() = default;
  explicit Sprite(std::shared_ptr<Texture2D> texture);
  Sprite(std::shared_ptr<Texture2D> texture, const SpriteDrawParams& params);

  void SetTexture(std::shared_ptr<Texture2D> texture);
  const std::shared_ptr<Texture2D>& GetTexture() const noexcept;
  bool IsValid() const noexcept;

  void SetPosition(const math::Vector2f& position) noexcept;
  const math::Vector2f& GetPosition() const noexcept;

  void SetOrigin(const math::Vector2f& origin) noexcept;
  const math::Vector2f& GetOrigin() const noexcept;

  void SetScale(const math::Vector2f& scale) noexcept;
  const math::Vector2f& GetScale() const noexcept;

  void SetRotation(float radians) noexcept;
  float GetRotation() const noexcept;

  void SetSourceRect(const std::optional<math::RectF>& rect) noexcept;
  const std::optional<math::RectF>& GetSourceRect() const noexcept;

  void SetTint(const Color& color) noexcept;
  const Color& GetTint() const noexcept;

  SpriteDrawParams& Params() noexcept { return params_; }
  const SpriteDrawParams& Params() const noexcept { return params_; }

  void Draw(Renderer2D& renderer) const;

 private:
  std::shared_ptr<Texture2D> texture_{};
  SpriteDrawParams params_{};
};

/**
 * @brief Single animation frame extracted from a sprite sheet.
 */
struct AnimationFrame {
  math::RectF source;
  float duration{0.1f};  // seconds
};

/**
 * @brief Time-based animation helper cycling through sprite frames.
 */
class Animation {
 public:
  Animation() = default;
  explicit Animation(std::vector<AnimationFrame> frames, bool looping = true);

  void SetFrames(std::vector<AnimationFrame> frames);
  void AddFrame(const AnimationFrame& frame);
  void ClearFrames();

  std::size_t GetFrameCount() const noexcept;
  std::size_t GetCurrentFrameIndex() const noexcept;

  void SetLooping(bool looping) noexcept;
  bool IsLooping() const noexcept { return looping_; }

  void Play() noexcept;
  void Pause() noexcept;
  void Stop() noexcept;
  bool IsPlaying() const noexcept { return playing_; }

  void Reset() noexcept;

  void Update(float delta_seconds);

  std::optional<math::RectF> GetCurrentFrameRect() const noexcept;
  bool IsFinished() const noexcept { return finished_once_; }
  bool Empty() const noexcept { return frames_.empty(); }

 private:
  void AdvanceFrame();

  std::vector<AnimationFrame> frames_;
  std::size_t current_frame_{0};
  float time_in_frame_{0.0f};
  bool looping_{true};
  bool playing_{false};
  bool finished_once_{false};
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_SPRITE_H_
