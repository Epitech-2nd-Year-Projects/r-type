#ifndef ENGINE_RENDER_LAYER_H_
#define ENGINE_RENDER_LAYER_H_

#include <array>
#include <cstddef>

namespace engine::render {

/**
 * @brief Fixed rendering layers controlling draw order and parallax.
 */
enum class RenderLayer : std::size_t {
  kBackground = 0,
  kMidground = 1,
  kForeground = 2,
};

inline constexpr std::size_t kRenderLayerCount = 3;

inline constexpr std::array<RenderLayer, kRenderLayerCount>
    kRenderLayerDrawOrder{RenderLayer::kBackground, RenderLayer::kMidground,
                          RenderLayer::kForeground};

inline constexpr std::array<float, kRenderLayerCount> kRenderLayerParallax{
    0.5f, 1.0f, 1.25f};

inline constexpr std::size_t ToLayerIndex(RenderLayer layer) {
  return static_cast<std::size_t>(layer);
}

inline constexpr float GetLayerParallax(RenderLayer layer) {
  return kRenderLayerParallax[ToLayerIndex(layer)];
}

}  // namespace engine::render

#endif  // ENGINE_RENDER_LAYER_H_
