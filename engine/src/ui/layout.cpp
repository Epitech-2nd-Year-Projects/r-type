#include <algorithm>
#include <utility>

#include "engine/render/renderer2d.h"
#include "engine/ui/canvas.h"
#include "engine/ui/layouts.h"
#include "engine/ui/text.h"

namespace engine::ui {

namespace {

float ClampNonNegative(float value) { return std::max(0.0f, value); }

Alignment2D OverrideForAxis(Axis axis, const Alignment2D& alignment,
                            const Alignment2D& override_alignment) {
  Alignment2D result = alignment;
  if (axis == Axis::kVertical) {
    result.horizontal = override_alignment.horizontal;
  } else {
    result.vertical = override_alignment.vertical;
  }
  return result;
}

math::RectF ApplyPadding(const math::RectF& rect, const Insets& padding) {
  const float x = rect.top_left_x_ + padding.left;
  const float y = rect.top_left_y_ + padding.top;
  const float width = ClampNonNegative(rect.width_ - padding.Horizontal());
  const float height = ClampNonNegative(rect.height_ - padding.Vertical());
  return {x, y, width, height};
}

}  // namespace

LayoutValue LayoutValue::Auto() { return LayoutValue(LayoutUnit::kAuto, 0.0f); }

LayoutValue LayoutValue::Pixels(float value) {
  return LayoutValue(LayoutUnit::kPixels, value);
}

LayoutValue LayoutValue::Percent(float value) {
  return LayoutValue(LayoutUnit::kPercent, value);
}

LayoutValue::LayoutValue(LayoutUnit unit, float value)
    : unit_(unit), value_(value) {}

Insets Insets::Uniform(float value) {
  return Insets{value, value, value, value};
}

Anchor Anchor::TopLeft() { return Anchor{0.0f, 0.0f}; }

Anchor Anchor::Center() { return Anchor{0.5f, 0.5f}; }

Anchor Anchor::BottomRight() { return Anchor{1.0f, 1.0f}; }

FontSize::FontSize() : unit_(FontSizeUnit::kPixels), value_(16.0f) {}

FontSize FontSize::Pixels(float value) {
  FontSize size;
  size.unit_ = FontSizeUnit::kPixels;
  size.value_ = value;
  return size;
}

FontSize FontSize::RelativeWidth(float ratio) {
  FontSize size;
  size.unit_ = FontSizeUnit::kViewportWidth;
  size.value_ = ratio;
  return size;
}

FontSize FontSize::RelativeHeight(float ratio) {
  FontSize size;
  size.unit_ = FontSizeUnit::kViewportHeight;
  size.value_ = ratio;
  return size;
}

FontSize FontSize::RelativeMin(float ratio) {
  FontSize size;
  size.unit_ = FontSizeUnit::kViewportMin;
  size.value_ = ratio;
  return size;
}

float FontSize::Resolve(const math::Vector2f& viewport) const {
  switch (unit_) {
    case FontSizeUnit::kPixels:
      return value_;
    case FontSizeUnit::kViewportWidth:
      return viewport.x * value_;
    case FontSizeUnit::kViewportHeight:
      return viewport.y * value_;
    case FontSizeUnit::kViewportMin:
      return std::min(viewport.x, viewport.y) * value_;
  }
  return value_;
}

math::Vector2f UIElement::Measure(const LayoutContext& context,
                                  const math::Vector2f& available_space) {
  const float available_width =
      ClampNonNegative(available_space.x - layout_.margin.Horizontal());
  const float available_height =
      ClampNonNegative(available_space.y - layout_.margin.Vertical());
  math::Vector2f content =
      ComputeContentSize(context, {available_width, available_height});

  const float resolved_width = ResolveSizeForAxis(
      layout_.size.width, available_width, content.x,
      layout_.alignment.horizontal == HorizontalAlignment::kStretch);
  const float resolved_height = ResolveSizeForAxis(
      layout_.size.height, available_height, content.y,
      layout_.alignment.vertical == VerticalAlignment::kStretch);

  measured_content_size_ = {resolved_width, resolved_height};
  measured_with_margin_ = {resolved_width + layout_.margin.Horizontal(),
                           resolved_height + layout_.margin.Vertical()};
  measured_ = true;
  return measured_with_margin_;
}

void UIElement::Arrange(const LayoutContext& context,
                        const math::RectF& bounds) {
  if (!measured_) {
    Measure(context, {bounds.width_, bounds.height_});
  }

  const float available_width =
      ClampNonNegative(bounds.width_ - layout_.margin.Horizontal());
  const float available_height =
      ClampNonNegative(bounds.height_ - layout_.margin.Vertical());

  const float resolved_width = ResolveSizeForAxis(
      layout_.size.width, available_width, measured_content_size_.x,
      layout_.alignment.horizontal == HorizontalAlignment::kStretch);
  const float resolved_height = ResolveSizeForAxis(
      layout_.size.height, available_height, measured_content_size_.y,
      layout_.alignment.vertical == VerticalAlignment::kStretch);

  const float x_offset =
      layout_.margin.left + AlignOffset(available_width, resolved_width,
                                        layout_.alignment.horizontal);
  const float y_offset =
      layout_.margin.top + AlignOffset(available_height, resolved_height,
                                       layout_.alignment.vertical);

  frame_ =
      math::RectF(bounds.top_left_x_ + x_offset, bounds.top_left_y_ + y_offset,
                  resolved_width, resolved_height);
  OnLayoutUpdated(context);
}

float UIElement::ResolveSizeForAxis(const LayoutValue& value, float available,
                                    float content, bool stretch) const {
  const float available_non_negative = ClampNonNegative(available);
  switch (value.unit()) {
    case LayoutUnit::kPixels:
      if (available_non_negative <= 0.0f) {
        return ClampNonNegative(value.value());
      }
      return std::clamp(value.value(), 0.0f, available_non_negative);
    case LayoutUnit::kPercent: {
      const float target = available_non_negative * value.value();
      const float clamped = ClampNonNegative(target);
      return available_non_negative > 0.0f
                 ? std::min(clamped, available_non_negative)
                 : clamped;
    }
    case LayoutUnit::kAuto:
      if (stretch && available_non_negative > 0.0f) {
        return available_non_negative;
      }
      const float desired = content < 0.0f ? 0.0f : content;
      if (available_non_negative > 0.0f) {
        return std::min(desired, available_non_negative);
      }
      return desired;
  }
  return content < 0.0f ? 0.0f : content;
}

float UIElement::AlignOffset(float available, float size,
                             HorizontalAlignment alignment) const {
  const float space = available - size;
  if (space <= 0.0f) return 0.0f;
  switch (alignment) {
    case HorizontalAlignment::kStart:
    case HorizontalAlignment::kStretch:
      return 0.0f;
    case HorizontalAlignment::kCenter:
      return space * 0.5f;
    case HorizontalAlignment::kEnd:
      return space;
  }
  return 0.0f;
}

float UIElement::AlignOffset(float available, float size,
                             VerticalAlignment alignment) const {
  const float space = available - size;
  if (space <= 0.0f) return 0.0f;
  switch (alignment) {
    case VerticalAlignment::kStart:
    case VerticalAlignment::kStretch:
      return 0.0f;
    case VerticalAlignment::kCenter:
      return space * 0.5f;
    case VerticalAlignment::kEnd:
      return space;
  }
  return 0.0f;
}

void UIContainer::AddChild(std::shared_ptr<UIElement> child) {
  if (child) {
    children_.push_back(std::move(child));
  }
}

void UIContainer::Draw(render::Renderer2D& renderer) const {
  DrawContent(renderer);
  for (const auto& child : children_) {
    if (child) {
      child->Draw(renderer);
    }
  }
}

void UIContainer::DrawContent(render::Renderer2D& /*renderer*/) const {}

math::Vector2f BoxElement::ComputeContentSize(
    const LayoutContext& /*context*/,
    const math::Vector2f& /*available_space*/) {
  return {0.0f, 0.0f};
}

void BoxElement::OnLayoutUpdated(const LayoutContext& /*context*/) {
  if (layout_callback_) {
    layout_callback_(Frame());
  }
}

void BoxElement::Draw(render::Renderer2D& renderer) const {
  if (background_.has_value()) {
    renderer.DrawRect(Frame(), *background_);
  }
}

void BoxElement::SetBackground(render::Color color) { background_ = color; }

void BoxElement::SetLayoutCallback(
    std::function<void(const math::RectF&)> callback) {
  layout_callback_ = std::move(callback);
}

StackContainer::StackContainer(Axis axis) : axis_(axis) {
  child_alignment_.horizontal = HorizontalAlignment::kStretch;
  child_alignment_.vertical = VerticalAlignment::kCenter;
}

math::Vector2f StackContainer::ComputeContentSize(
    const LayoutContext& context, const math::Vector2f& available_space) {
  const math::Vector2f inner_space{
      ClampNonNegative(available_space.x - padding_.Horizontal()),
      ClampNonNegative(available_space.y - padding_.Vertical())};

  float main_extent = 0.0f;
  float cross_extent = 0.0f;
  std::size_t count = 0;

  for (const auto& child : Children()) {
    if (!child) continue;
    const Alignment2D original = child->Layout().alignment;
    if (use_child_alignment_) {
      child->Layout().alignment =
          OverrideForAxis(axis_, original, child_alignment_);
    }
    const math::Vector2f measured = child->Measure(context, inner_space);
    if (use_child_alignment_) {
      child->Layout().alignment = original;
    }

    const float main = axis_ == Axis::kVertical ? measured.y : measured.x;
    const float cross = axis_ == Axis::kVertical ? measured.x : measured.y;
    main_extent += main;
    cross_extent = std::max(cross_extent, cross);
    ++count;
  }

  if (count > 1) {
    main_extent += spacing_ * static_cast<float>(count - 1);
  }

  if (axis_ == Axis::kVertical) {
    return {cross_extent + padding_.Horizontal(),
            main_extent + padding_.Vertical()};
  }
  return {main_extent + padding_.Horizontal(),
          cross_extent + padding_.Vertical()};
}

void StackContainer::OnLayoutUpdated(const LayoutContext& context) {
  const math::RectF inner = ApplyPadding(ContentRect(), padding_);
  if (axis_ == Axis::kVertical) {
    ArrangeVertical(context, inner);
  } else {
    ArrangeHorizontal(context, inner);
  }
}

void StackContainer::ArrangeVertical(const LayoutContext& context,
                                     const math::RectF& content_bounds) {
  float total_height = 0.0f;
  std::size_t count = 0;
  for (const auto& child : Children()) {
    if (!child) continue;
    total_height += child->MeasuredSizeWithMargin().y;
    ++count;
  }
  if (count > 1) {
    total_height += spacing_ * static_cast<float>(count - 1);
  }

  const float start_offset = [&]() {
    const float remaining = content_bounds.height_ - total_height;
    switch (main_alignment_) {
      case StackAlignment::kStart:
        return 0.0f;
      case StackAlignment::kCenter:
        return remaining > 0.0f ? remaining * 0.5f : 0.0f;
      case StackAlignment::kEnd:
        return remaining > 0.0f ? remaining : 0.0f;
    }
    return 0.0f;
  }();

  float cursor = content_bounds.top_left_y_ + start_offset;
  for (const auto& child : Children()) {
    if (!child) continue;
    const Alignment2D original = child->Layout().alignment;
    if (use_child_alignment_) {
      child->Layout().alignment =
          OverrideForAxis(axis_, original, child_alignment_);
    }

    const float child_height = child->MeasuredSizeWithMargin().y;
    const math::RectF slot{content_bounds.top_left_x_, cursor,
                           content_bounds.width_, child_height};
    child->Arrange(context, slot);
    if (use_child_alignment_) {
      child->Layout().alignment = original;
    }
    cursor += child_height + spacing_;
  }
}

void StackContainer::ArrangeHorizontal(const LayoutContext& context,
                                       const math::RectF& content_bounds) {
  float total_width = 0.0f;
  std::size_t count = 0;
  for (const auto& child : Children()) {
    if (!child) continue;
    total_width += child->MeasuredSizeWithMargin().x;
    ++count;
  }
  if (count > 1) {
    total_width += spacing_ * static_cast<float>(count - 1);
  }

  const float start_offset = [&]() {
    const float remaining = content_bounds.width_ - total_width;
    switch (main_alignment_) {
      case StackAlignment::kStart:
        return 0.0f;
      case StackAlignment::kCenter:
        return remaining > 0.0f ? remaining * 0.5f : 0.0f;
      case StackAlignment::kEnd:
        return remaining > 0.0f ? remaining : 0.0f;
    }
    return 0.0f;
  }();

  float cursor = content_bounds.top_left_x_ + start_offset;
  for (const auto& child : Children()) {
    if (!child) continue;
    const Alignment2D original = child->Layout().alignment;
    if (use_child_alignment_) {
      child->Layout().alignment =
          OverrideForAxis(axis_, original, child_alignment_);
    }

    const float child_width = child->MeasuredSizeWithMargin().x;
    const math::RectF slot{cursor, content_bounds.top_left_y_, child_width,
                           content_bounds.height_};
    child->Arrange(context, slot);
    if (use_child_alignment_) {
      child->Layout().alignment = original;
    }
    cursor += child_width + spacing_;
  }
}

void AnchorContainer::AddAnchoredChild(const std::shared_ptr<UIElement>& child,
                                       Anchor anchor,
                                       const math::Vector2f& offset) {
  if (!child) return;
  anchored_children_.push_back(AnchoredChild{child, anchor, offset});
  AddChild(child);
}

math::Vector2f AnchorContainer::ComputeContentSize(
    const LayoutContext& /*context*/, const math::Vector2f& available_space) {
  return {ClampNonNegative(available_space.x),
          ClampNonNegative(available_space.y)};
}

void AnchorContainer::OnLayoutUpdated(const LayoutContext& context) {
  const math::RectF inner = ApplyPadding(ContentRect(), padding_);
  const math::Vector2f available{inner.width_, inner.height_};

  for (auto& entry : anchored_children_) {
    if (!entry.element) {
      continue;
    }
    entry.element->Measure(context, available);
    const math::Vector2f measured = entry.element->MeasuredSizeWithMargin();

    const math::Vector2f anchor_point{
        inner.top_left_x_ + entry.anchor.x * inner.width_,
        inner.top_left_y_ + entry.anchor.y * inner.height_};

    const math::Vector2f pivot = entry.element->Layout().pivot;

    const float origin_x =
        anchor_point.x + entry.offset.x - measured.x * pivot.x;
    const float origin_y =
        anchor_point.y + entry.offset.y - measured.y * pivot.y;

    const math::RectF slot{origin_x, origin_y, measured.x, measured.y};
    entry.element->Arrange(context, slot);
  }
}

TextElement::TextElement(std::string text, FontSize font_size,
                         render::Color color)
    : text_(std::move(text)), font_size_(font_size), color_(color) {}

void TextElement::SetText(std::string text) { text_ = std::move(text); }

void TextElement::SetFontSize(FontSize size) { font_size_ = size; }

math::Vector2f TextElement::ComputeContentSize(
    const LayoutContext& context, const math::Vector2f& /*available_space*/) {
  resolved_font_size_ = font_size_.Resolve(context.viewport_size);
  if (text_.empty()) {
    return {0.0f, 0.0f};
  }
  return context.renderer.MeasureText(text_, resolved_font_size_);
}

void TextElement::Draw(render::Renderer2D& renderer) const {
  if (text_.empty()) {
    return;
  }
  const float font_size =
      resolved_font_size_ > 0.0f
          ? resolved_font_size_
          : font_size_.Resolve({ContentRect().width_, ContentRect().height_});
  renderer.DrawText(text_,
                    {ContentRect().top_left_x_, ContentRect().top_left_y_},
                    font_size, color_);
}

LayoutContext Canvas::BuildContext(render::Renderer2D& renderer) const {
  return LayoutContext{renderer, viewport_size_};
}

void Canvas::Layout(render::Renderer2D& renderer) {
  if (!root_) {
    return;
  }
  LayoutContext context = BuildContext(renderer);
  root_->Measure(context, viewport_size_);
  root_->Arrange(context,
                 math::RectF(0.0f, 0.0f, viewport_size_.x, viewport_size_.y));
}

void Canvas::Draw(render::Renderer2D& renderer) const {
  if (root_) {
    root_->Draw(renderer);
  }
}

}  // namespace engine::ui
