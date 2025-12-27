/**
 * @file client_asset_manager
 * @brief Client asset cache and path resolver
 */

#ifndef CLIENT_CLIENT_ASSET_MANAGER_H_
#define CLIENT_CLIENT_ASSET_MANAGER_H_

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace engine::audio {
class AudioEngine;
}  // namespace engine::audio

namespace engine::render {
class Renderer2D;
class Texture2D;
}  // namespace engine::render

namespace client {

/**
 * @brief Client asset cache and resolver for fonts textures and sfx
 */
class ClientAssetManager {
 public:
  /**
   * @brief Create a client asset manager
   * @param renderer Renderer reference
   */
  explicit ClientAssetManager(engine::render::Renderer2D& renderer);

  /**
   * @brief Provide an audio engine for sfx preloading
   * @param audio Audio engine handle
   */
  void SetAudioEngine(std::shared_ptr<engine::audio::AudioEngine> audio);

  /**
   * @brief Load a font if not cached
   * @param name Font identifier
   * @param relative_path Font asset path
   */
  void LoadFont(std::string_view name, std::string_view relative_path);

  /**
   * @brief Get a cached texture or load it
   * @param relative_path Texture asset path
   * @return Texture handle if available
   */
  std::shared_ptr<engine::render::Texture2D> GetTexture(
      std::string_view relative_path);

  /**
   * @brief Resolve and cache an sfx path
   * @param relative_path Sfx asset path
   * @return Resolved path string
   */
  std::string GetSfxPath(std::string_view relative_path);

  /**
   * @brief Preload common menu assets
   */
  void PreloadMenuAssets();

 private:
  void PreloadSfx(std::string_view relative_path);
  void PreloadTextureSequence(std::string_view prefix,
                              std::string_view extension, int count);

  engine::render::Renderer2D& renderer_;
  std::weak_ptr<engine::audio::AudioEngine> audio_engine_;
  std::unordered_map<std::string, std::string> font_paths_;
  std::unordered_map<std::string, std::shared_ptr<engine::render::Texture2D>>
      textures_;
  std::unordered_map<std::string, std::string> sfx_paths_;
};

}  // namespace client

#endif  // CLIENT_CLIENT_ASSET_MANAGER_H_
