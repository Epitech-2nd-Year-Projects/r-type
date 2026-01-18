#ifndef TESTS_CLIENT_FAKE_RENDERER_H_
#define TESTS_CLIENT_FAKE_RENDERER_H_

#include <memory>
#include <string>
#include <vector>

#include "engine/render/renderer2d.h"

namespace {

class FakeTexture : public engine::render::Texture2D {
 public:
  explicit FakeTexture(std::string id, engine::math::Vector2i size)
      : id_(std::move(id)), size_(size) {}

  engine::math::Vector2i GetSize() const override { return size_; }

  std::string id_;
  engine::math::Vector2i size_;
};

class FakeRenderer : public engine::render::Renderer2D {
 public:
  struct Call {
    std::string texture;
    engine::render::SpriteDrawParams params;
  };

  struct LineCall {
    engine::math::Vector2f start;
    engine::math::Vector2f end;
    float thickness;
    engine::render::Color color;
  };

  void DrawRect(const engine::math::RectF& rect,
                const engine::render::Color&) override {
    rects.push_back(rect);
  }
  void DrawCircle(const engine::math::Vector2f&, float,
                  const engine::render::Color&) override {}
  void DrawLine(const engine::math::Vector2f& start,
                const engine::math::Vector2f& end, float thickness,
                const engine::render::Color& color) override {
    line_calls.push_back({start, end, thickness, color});
  }
  void DrawRing(const engine::math::Vector2f&, float, float, float, float, int,
                const engine::render::Color&) override {}

  void DrawTexture(const engine::render::Texture2D& texture,
                   const engine::render::SpriteDrawParams& params) override {
    const auto* fake = dynamic_cast<const FakeTexture*>(&texture);
    if (fake != nullptr) {
      calls.push_back(Call{fake->id_, params});
    }
  }

  void DrawText(std::string_view, const engine::math::Vector2f&, float,
                const engine::render::Color&) override {}

  engine::math::Vector2f MeasureText(std::string_view, float) override {
    return {0.0f, 0.0f};
  }

  std::shared_ptr<engine::render::Texture2D> LoadTextureFromFile(
      const std::string& path) override {
    auto texture =
        std::make_shared<FakeTexture>(path, engine::math::Vector2i{64, 64});
    loaded.push_back(path);
    return texture;
  }

  void LoadFont(const std::string&, const std::string&) override {}
  void SetFont(const std::string&) override {}
  void Flush() override {}

  std::vector<std::string> loaded;
  std::vector<Call> calls;
  std::vector<engine::math::RectF> rects;
  std::vector<LineCall> line_calls;
};

}  // namespace

#endif  // TESTS_CLIENT_FAKE_RENDERER_H_
