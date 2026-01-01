#include "client_asset_manager.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

#include "audio_paths.h"
#include "constants/ui_constants.h"
#include "engine/audio/audio_engine.h"
#include "engine/render/renderer2d.h"

namespace client {

ClientAssetManager::ClientAssetManager(engine::render::Renderer2D& renderer)
    : renderer_(renderer) {}

void ClientAssetManager::SetAudioEngine(
    std::shared_ptr<engine::audio::AudioEngine> audio) {
  audio_engine_ = std::move(audio);
}

void ClientAssetManager::LoadFont(std::string_view name,
                                  std::string_view relative_path) {
  const std::string resolved = ResolveAssetPath(relative_path);
  if (resolved.empty()) {
    return;
  }
  const std::string key(name);
  auto it = font_paths_.find(key);
  if (it != font_paths_.end() && it->second == resolved) {
    return;
  }
  font_paths_[key] = resolved;
  renderer_.LoadFont(std::string(name), resolved);
}

std::shared_ptr<engine::render::Texture2D> ClientAssetManager::GetTexture(
    std::string_view relative_path) {
  const std::string resolved = ResolveAssetPath(relative_path);
  if (resolved.empty()) {
    return nullptr;
  }
  auto it = textures_.find(resolved);
  if (it != textures_.end()) {
    return it->second;
  }
  auto texture = renderer_.LoadTextureFromFile(resolved);
  if (texture) {
    textures_.emplace(resolved, texture);
  }
  return texture;
}

std::string ClientAssetManager::GetSfxPath(std::string_view relative_path) {
  const std::string key(relative_path);
  auto it = sfx_paths_.find(key);
  if (it != sfx_paths_.end()) {
    return it->second;
  }
  std::string resolved = ResolveAssetPath(relative_path);
  if (resolved.empty()) {
    return {};
  }
  sfx_paths_.emplace(key, resolved);
  return resolved;
}

void ClientAssetManager::PreloadMenuAssets() {
  LoadFont(constants::ui::kTitleFont, constants::ui::kTitleFontPath);
  LoadFont(constants::ui::kBodyFont, constants::ui::kBodyFontPath);

  GetTexture(constants::ui::kButtonTextureLargePath);
  GetTexture(constants::ui::kButtonTextureSmallPath);
  GetTexture(constants::ui::MainMenu::kTitleTexturePath);

  const int pointer_frames =
      std::max(constants::ui::MainMenu::kPointerFrameCount,
               constants::ui::OptionsMenu::kPointerFrameCount);
  PreloadTextureSequence(constants::ui::kMenuPointerFramePrefix,
                         constants::ui::kMenuPointerFrameExtension,
                         pointer_frames);
  PreloadTextureSequence(constants::ui::OptionsMenu::kWarningFramePrefix,
                         constants::ui::OptionsMenu::kWarningFrameExtension,
                         constants::ui::OptionsMenu::kWarningFrameCount);
  PreloadTextureSequence(constants::ui::Pause::kTopFleurFramePrefix,
                         constants::ui::Pause::kFleurFrameExtension,
                         constants::ui::Pause::kFleurFrameCount);
  PreloadTextureSequence(constants::ui::Pause::kBottomFleurFramePrefix,
                         constants::ui::Pause::kFleurFrameExtension,
                         constants::ui::Pause::kFleurFrameCount);

  PreloadSfx(constants::ui::kMenuHoverSfxPath);
  PreloadSfx(constants::ui::kMenuClickSfxPath);
}

void ClientAssetManager::PreloadSfx(std::string_view relative_path) {
  const auto path = GetSfxPath(relative_path);
  if (path.empty()) {
    return;
  }
  auto audio = audio_engine_.lock();
  if (!audio) {
    return;
  }
  const float original_volume = audio->GetSfxVolume();
  audio->SetSfxVolume(0.0f);
  audio->PlaySoundEffect(path);
  audio->SetSfxVolume(original_volume);
}

void ClientAssetManager::PreloadTextureSequence(std::string_view prefix,
                                                std::string_view extension,
                                                int count) {
  if (count <= 0) {
    return;
  }
  for (int i = 0; i < count; ++i) {
    std::ostringstream path;
    path << prefix << std::setw(4) << std::setfill('0') << i << extension;
    GetTexture(path.str());
  }
}

}  // namespace client
