#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "engine/render/color.h"
#include "engine/render/sprite.h"
#include "engine/ui/canvas.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"

namespace {

class FakeRenderer : public engine::render::Renderer2D {
 public:
  float last_measured_font_size{0.0f};

  void DrawRect(const engine::math::RectF& /*rect*/,
                const engine::render::Color& /*color*/) override {}

  void DrawCircle(const engine::math::Vector2f& /*center*/, float /*radius*/,
                  const engine::render::Color& /*color*/) override {}

  void DrawLine(const engine::math::Vector2f& /*start*/,
                const engine::math::Vector2f& /*end*/, float /*thickness*/,
                const engine::render::Color& /*color*/) override {}

  void DrawTexture(const engine::render::Texture2D& /*texture*/,
                   const engine::render::SpriteDrawParams& /*params*/) override {
  }

  void DrawText(std::string_view /*text*/,
                const engine::math::Vector2f& /*position*/, float /*font_size*/,
                const engine::render::Color& /*color*/) override {}

  engine::math::Vector2f MeasureText(std::string_view text,
                                     float font_size) override {
    last_measured_font_size = font_size;
    const float width =
        static_cast<float>(text.size()) * font_size * 0.5f;
    return {width, font_size};
  }

  std::shared_ptr<engine::render::Texture2D> LoadTextureFromFile(
      const std::string& /*path*/) override {
    return nullptr;
  }

  void LoadFont(const std::string& /*name*/,
                const std::string& /*path*/) override {}

  void SetFont(const std::string& /*name*/) override {}

  void Flush() override {}
};

}  // namespace

TEST(UiLayoutTest, VerticalStackCentersChildren) {
  FakeRenderer renderer;
  engine::ui::Canvas canvas({800.0f, 600.0f});

  auto root = std::make_shared<engine::ui::StackContainer>(
      engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  root->SetSpacing(10.0f);
  root->SetMainAlignment(engine::ui::StackAlignment::kCenter);
  root->SetChildAlignment({engine::ui::HorizontalAlignment::kCenter,
                           engine::ui::VerticalAlignment::kCenter});

  auto first = std::make_shared<engine::ui::TextElement>(
      "Hello", engine::ui::FontSize::Pixels(20.0f),
      engine::render::Color::White());
  auto second = std::make_shared<engine::ui::TextElement>(
      "World", engine::ui::FontSize::Pixels(20.0f),
      engine::render::Color::White());

  root->AddChild(first);
  root->AddChild(second);
  canvas.SetRoot(root);
  canvas.Layout(renderer);

  EXPECT_FLOAT_EQ(first->Frame().top_left_y_, 275.0f);
  EXPECT_FLOAT_EQ(second->Frame().top_left_y_, 305.0f);
  EXPECT_FLOAT_EQ(first->Frame().top_left_x_, 375.0f);
  EXPECT_FLOAT_EQ(second->Frame().top_left_x_, 375.0f);
}

TEST(UiLayoutTest, AnchorRespectsPivotAndOffset) {
  FakeRenderer renderer;
  engine::ui::Canvas canvas({800.0f, 600.0f});

  auto root = std::make_shared<engine::ui::AnchorContainer>();
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);

  auto box = std::make_shared<engine::ui::BoxElement>();
  box->Layout().size.width = engine::ui::LayoutValue::Pixels(100.0f);
  box->Layout().size.height = engine::ui::LayoutValue::Pixels(40.0f);
  box->Layout().pivot = {1.0f, 1.0f};

  root->AddAnchoredChild(box, engine::ui::Anchor::BottomRight(),
                         {-20.0f, -10.0f});

  canvas.SetRoot(root);
  canvas.Layout(renderer);

  EXPECT_FLOAT_EQ(box->Frame().top_left_x_, 680.0f);
  EXPECT_FLOAT_EQ(box->Frame().top_left_y_, 550.0f);
}

TEST(UiLayoutTest, RelativeFontSizeUsesViewport) {
  FakeRenderer renderer;
  engine::ui::Canvas canvas({800.0f, 600.0f});

  auto root = std::make_shared<engine::ui::StackContainer>(
      engine::ui::Axis::kVertical);
  root->Layout().size.width = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().size.height = engine::ui::LayoutValue::Percent(1.0f);
  root->Layout().alignment.horizontal =
      engine::ui::HorizontalAlignment::kStretch;
  root->Layout().alignment.vertical = engine::ui::VerticalAlignment::kStretch;
  auto text = std::make_shared<engine::ui::TextElement>(
      "Scale", engine::ui::FontSize::RelativeWidth(0.05f),
      engine::render::Color::White());
  root->AddChild(text);
  canvas.SetRoot(root);

  canvas.Layout(renderer);

  EXPECT_FLOAT_EQ(renderer.last_measured_font_size, 40.0f);
  EXPECT_FLOAT_EQ(text->Frame().height_, 40.0f);
}
