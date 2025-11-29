#include "engine/render/sprite.h"

#include <algorithm>
#include <utility>

namespace engine::render {

namespace {

constexpr float kMinimumFrameDuration = 0.0001f;

float SanitizeDuration(float duration) {
  return duration <= 0.0f ? kMinimumFrameDuration : duration;
}

}  // namespace

Sprite::Sprite(std::shared_ptr<Texture2D> texture)
    : texture_(std::move(texture)) {}

Sprite::Sprite(std::shared_ptr<Texture2D> texture,
               const SpriteDrawParams& params)
    : texture_(std::move(texture)), params_(params) {}

void Sprite::SetTexture(std::shared_ptr<Texture2D> texture) {
  texture_ = std::move(texture);
}

const std::shared_ptr<Texture2D>& Sprite::GetTexture() const noexcept {
  return texture_;
}

bool Sprite::IsValid() const noexcept { return texture_ != nullptr; }

void Sprite::SetPosition(const math::Vector2f& position) noexcept {
  params_.position = position;
}

const math::Vector2f& Sprite::GetPosition() const noexcept {
  return params_.position;
}

void Sprite::SetOrigin(const math::Vector2f& origin) noexcept {
  params_.origin = origin;
}

const math::Vector2f& Sprite::GetOrigin() const noexcept {
  return params_.origin;
}

void Sprite::SetScale(const math::Vector2f& scale) noexcept {
  params_.scale = scale;
}

const math::Vector2f& Sprite::GetScale() const noexcept {
  return params_.scale;
}

void Sprite::SetRotation(float radians) noexcept { params_.rotation = radians; }

float Sprite::GetRotation() const noexcept { return params_.rotation; }

void Sprite::SetSourceRect(const std::optional<math::RectF>& rect) noexcept {
  params_.source = rect;
}

const std::optional<math::RectF>& Sprite::GetSourceRect() const noexcept {
  return params_.source;
}

void Sprite::SetTint(const Color& color) noexcept { params_.tint = color; }

const Color& Sprite::GetTint() const noexcept { return params_.tint; }

void Sprite::Draw(Renderer2D& renderer) const {
  if (!texture_) {
    return;
  }
  renderer.DrawTexture(*texture_, params_);
}

Animation::Animation(std::vector<AnimationFrame> frames, bool looping)
    : frames_(std::move(frames)), looping_(looping) {
  for (auto& frame : frames_) {
    frame.duration = SanitizeDuration(frame.duration);
  }
}

void Animation::SetFrames(std::vector<AnimationFrame> frames) {
  frames_ = std::move(frames);
  for (auto& frame : frames_) {
    frame.duration = SanitizeDuration(frame.duration);
  }
  Reset();
}

void Animation::AddFrame(const AnimationFrame& frame) {
  AnimationFrame sanitized = frame;
  sanitized.duration = SanitizeDuration(frame.duration);
  frames_.push_back(sanitized);
}

void Animation::ClearFrames() {
  frames_.clear();
  Reset();
  playing_ = false;
}

std::size_t Animation::GetFrameCount() const noexcept { return frames_.size(); }

std::size_t Animation::GetCurrentFrameIndex() const noexcept {
  if (frames_.empty()) {
    return 0;
  }
  return std::min(current_frame_, frames_.size() - 1);
}

void Animation::SetLooping(bool looping) noexcept { looping_ = looping; }

void Animation::Play() noexcept {
  if (frames_.empty()) {
    return;
  }
  playing_ = true;
  finished_once_ = false;
}

void Animation::Pause() noexcept { playing_ = false; }

void Animation::Stop() noexcept {
  playing_ = false;
  Reset();
}

void Animation::Reset() noexcept {
  current_frame_ = 0;
  time_in_frame_ = 0.0f;
  finished_once_ = false;
}

void Animation::Update(float delta_seconds) {
  if (!playing_ || frames_.empty() || delta_seconds <= 0.0f) {
    return;
  }

  time_in_frame_ += delta_seconds;

  while (playing_ &&
         time_in_frame_ >= frames_[GetCurrentFrameIndex()].duration) {
    time_in_frame_ -= frames_[GetCurrentFrameIndex()].duration;
    AdvanceFrame();
  }
}

std::optional<math::RectF> Animation::GetCurrentFrameRect() const noexcept {
  if (frames_.empty()) {
    return std::nullopt;
  }
  return frames_[GetCurrentFrameIndex()].source;
}

void Animation::AdvanceFrame() {
  if (frames_.empty()) {
    return;
  }

  if (current_frame_ + 1 < frames_.size()) {
    ++current_frame_;
    return;
  }

  if (looping_) {
    current_frame_ = 0;
  } else {
    current_frame_ = frames_.size() - 1;
    playing_ = false;
    finished_once_ = true;
    time_in_frame_ = 0.0f;
  }
}

}  // namespace engine::render
