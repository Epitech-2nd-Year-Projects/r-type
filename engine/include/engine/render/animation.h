#ifndef ENGINE_RENDER_ANIMATION_H_
#define ENGINE_RENDER_ANIMATION_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace engine::render {

/// Handle for a single animation clip loaded from a file.
class Animation {
 public:
  virtual ~Animation() = default;

  /// Get the total number of frames in this animation.
  virtual std::uint32_t GetFrameCount() const = 0;

  /// Get the animation name (from the file).
  virtual std::string_view GetName() const = 0;

  /// Get the bone count in this animation.
  virtual std::uint32_t GetBoneCount() const = 0;
};

/// Container for multiple animations loaded from a single file.
class AnimationSet {
 public:
  virtual ~AnimationSet() = default;

  /// Get the number of animations in this set.
  virtual std::size_t GetCount() const = 0;

  /// Get animation by index.
  virtual const Animation* GetAnimation(std::size_t index) const = 0;

  /// Find animation by name.
  virtual const Animation* FindByName(std::string_view name) const = 0;
};

}  // namespace engine::render

#endif  // ENGINE_RENDER_ANIMATION_H_
