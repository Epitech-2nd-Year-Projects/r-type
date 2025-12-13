#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "ecs/render_system.h"
#include "engine/ecs/registry.h"

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

  void DrawRect(const engine::math::RectF&, const engine::render::Color&) override {}
  void DrawCircle(const engine::math::Vector2f&, float,
                  const engine::render::Color&) override {}
  void DrawLine(const engine::math::Vector2f&, const engine::math::Vector2f&,
                float, const engine::render::Color&) override {}

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
};

}  // namespace

TEST(RenderSystemTest, PopulatesSpritesAndDraws) {
  engine::ecs::Registry registry;
  FakeRenderer renderer;
  client::ecs::RenderSystem render_system(registry, renderer);

  const auto entity = registry.SpawnEntity();
  registry.EmplaceComponent<client::ecs::PositionComponent>(entity, 10.0f,
                                                            20.0f);
  registry.EmplaceComponent<client::ecs::NetworkedEntityComponent>(
      entity, 1u, 1u, 0u);

  render_system.Render();

  const auto& sprites = registry.GetComponents<client::ecs::SpriteComponent>();
  ASSERT_LT(static_cast<std::size_t>(entity), sprites.size());
  ASSERT_TRUE(sprites[entity].has_value());
  EXPECT_FALSE(sprites[entity]->texture_id.empty());
  EXPECT_EQ(renderer.calls.size(), 1u);
  EXPECT_EQ(renderer.calls.front().texture, "assets/sprites/player.png");
  EXPECT_FLOAT_EQ(renderer.calls.front().params.position.x, 10.0f);
  EXPECT_FLOAT_EQ(renderer.calls.front().params.position.y, 20.0f);
}

TEST(RenderSystemTest, OrdersByLayer) {
  engine::ecs::Registry registry;
  FakeRenderer renderer;
  client::ecs::RenderSystem render_system(registry, renderer);

  const auto obstacle = registry.SpawnEntity();
  registry.EmplaceComponent<client::ecs::PositionComponent>(obstacle, 0.0f,
                                                            0.0f);
  registry.EmplaceComponent<client::ecs::NetworkedEntityComponent>(
      obstacle, 2u, 4u, 0u);

  const auto player = registry.SpawnEntity();
  registry.EmplaceComponent<client::ecs::PositionComponent>(player, 5.0f,
                                                            5.0f);
  registry.EmplaceComponent<client::ecs::NetworkedEntityComponent>(
      player, 3u, 1u, 0u);

  render_system.Render();

  ASSERT_EQ(renderer.calls.size(), 2u);
  EXPECT_EQ(renderer.calls[0].texture,
            "assets/sprites/obstacle_destructible.png");
  EXPECT_EQ(renderer.calls[1].texture, "assets/sprites/player.png");
}
