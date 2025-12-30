/**
 * @file ui_constants
 * @brief UI tokens and layout values
 */

#ifndef CLIENT_CONSTANTS_UI_CONSTANTS_H_
#define CLIENT_CONSTANTS_UI_CONSTANTS_H_

#include <chrono>
#include <cstddef>
#include <string_view>

#include "engine/render/color.h"

namespace client::constants::ui {

inline constexpr std::string_view kTitleFont = "title_font";
inline constexpr std::string_view kBodyFont = "body_font";
inline constexpr std::string_view kTitleFontPath =
    "assets/fonts/trajanpro_bold.otf";
inline constexpr std::string_view kBodyFontPath =
    "assets/fonts/Perpetua-Regular.otf";
inline constexpr std::string_view kMenuPointerFramePrefix =
    "assets/ui/main_menu_pointer_anim";
inline constexpr std::string_view kMenuPointerFrameExtension = ".png";
inline constexpr std::string_view kMenuHoverSfxPath =
    "assets/song/effects/change_selection.mp3";
inline constexpr std::string_view kMenuClickSfxPath =
    "assets/song/effects/button_confirm.mp3";
inline constexpr std::string_view kButtonTextureLargePath =
    "assets/ui/button_large.png";
inline constexpr std::string_view kButtonTextureSmallPath =
    "assets/ui/button_small.png";

inline constexpr engine::render::Color kButtonBaseColor =
    engine::render::Color::White();
inline constexpr engine::render::Color kButtonHoverColor =
    engine::render::Color::FromBytes(220, 220, 220);
inline constexpr engine::render::Color kButtonPressColor =
    engine::render::Color::FromBytes(180, 180, 180);

/**
 * @brief Main menu layout values
 */
struct MainMenu {
  static constexpr float kButtonHeight = 72.0f;
  static constexpr float kButtonWidth = 170.0f;
  static constexpr float kButtonTextScale = 0.46f;
  static constexpr float kRootPadding = 48.0f;
  static constexpr float kRootSpacing = 36.0f;
  static constexpr float kTitleSlotHeight = 300.0f;
  static constexpr float kTitleYOffset = -12.0f;
  static constexpr float kButtonColumnSpacing = 22.0f;
  static constexpr float kButtonSlotPadding = 8.0f;
  static constexpr float kButtonSlotInset = 4.0f;
  static constexpr float kPointerHeightFactor = 0.85f;
  static constexpr float kPointerFrameDuration = 0.06f;
  static constexpr float kPointerSpacing = 28.0f;
  static constexpr float kPointerScaleFactor = 0.6f;
  static constexpr float kTitleScaleFactor = 1.3f;
  static constexpr float kVersionFontSize = 18.0f;
  static constexpr float kVersionPaddingX = 24.0f;
  static constexpr float kVersionPaddingBottom = 18.0f;
  static constexpr engine::render::Color kVersionColor =
      engine::render::Color::FromBytes(180, 190, 210);
  static constexpr int kPointerFrameCount = 11;
  static constexpr std::string_view kTitleTexturePath =
      "assets/ui/main_menu_title.png";
  static constexpr std::string_view kBackgroundVideoPath =
      "assets/ui/main_menu_background.mp4";
};

/**
 * @brief Options menu layout values
 */
struct OptionsMenu {
  static constexpr float kButtonHeight = 72.0f;
  static constexpr float kButtonWidth = 170.0f;
  static constexpr float kButtonTextScale = 0.46f;
  static constexpr float kRootPadding = 48.0f;
  static constexpr float kRootPaddingTop = 40.0f;
  static constexpr float kRootSpacing = 14.0f;
  static constexpr float kButtonColumnSpacing = 20.0f;
  static constexpr float kButtonSlotPadding = 8.0f;
  static constexpr float kButtonSlotInset = 4.0f;
  static constexpr float kBackSlotMarginTop = 116.0f;
  static constexpr float kPointerHeightFactor = 0.85f;
  static constexpr float kPointerFrameDuration = 0.06f;
  static constexpr float kPointerSpacing = 28.0f;
  static constexpr float kPointerScaleFactor = 0.6f;
  static constexpr float kTitleScaleFactor = 1.5f;
  static constexpr float kWarningFrameDuration = 0.06f;
  static constexpr float kWarningSlotHeight = 50.0f;
  static constexpr int kWarningFrameCount = 9;
  static constexpr int kPointerFrameCount = 11;
  static constexpr std::string_view kWarningFramePrefix =
      "assets/ui/Warning_Fleur";
  static constexpr std::string_view kWarningFrameExtension = ".png";
};

/**
 * @brief Connecting layout values
 */
struct Connecting {
  static constexpr float kStatusFontScale = 0.04f;
};

/**
 * @brief HUD overlay layout values
 */
struct HudOverlay {
  static constexpr float kPanelMargin = 16.0f;
  static constexpr float kPanelPadding = 12.0f;
  static constexpr float kLineSpacing = 6.0f;
  static constexpr float kHeaderFontSize = 20.0f;
  static constexpr float kBodyFontSize = 18.0f;
  static constexpr float kIndicatorRadius = 7.0f;
  static constexpr float kIndicatorTextGap = 8.0f;
  static constexpr float kLatencyGoodThresholdMs = 80.0f;
  static constexpr float kLatencyWarningThresholdMs = 150.0f;
  static constexpr engine::render::Color kPanelBackground =
      engine::render::Color::FromBytes(10, 12, 16, 205);
  static constexpr engine::render::Color kHeaderColor =
      engine::render::Color::FromBytes(230, 235, 245);
  static constexpr engine::render::Color kBodyColor =
      engine::render::Color::FromBytes(214, 222, 230);
  static constexpr engine::render::Color kMutedColor =
      engine::render::Color::FromBytes(150, 160, 170);
  static constexpr engine::render::Color kLocalColor =
      engine::render::Color::FromBytes(120, 190, 255);
  static constexpr engine::render::Color kConnectedColor =
      engine::render::Color::FromBytes(84, 199, 136);
  static constexpr engine::render::Color kWarningColor =
      engine::render::Color::FromBytes(236, 195, 86);
  static constexpr engine::render::Color kProblemColor =
      engine::render::Color::FromBytes(214, 89, 82);
  static constexpr engine::render::Color kOfflineColor =
      engine::render::Color::FromBytes(120, 130, 140);
};

/**
 * @brief Disconnected layout values
 */
struct Disconnected {
  static constexpr float kRootSpacing = 14.0f;
  static constexpr float kTitleFontScale = 0.06f;
  static constexpr float kReasonFontScale = 0.03f;
  static constexpr float kActionFontScale = 0.03f;
  static constexpr engine::render::Color kTitleColor =
      engine::render::Color::FromBytes(255, 0, 0);
};

/**
 * @brief Settings layout values
 */
struct Settings {
  static constexpr float kRootPadding = 48.0f;
  static constexpr float kRootSpacing = 20.0f;
  static constexpr float kContentSpacing = 15.0f;
  static constexpr float kTitleFontScale = 0.1f;
  static constexpr float kControlsTitleFontSize = 28.0f;
  static constexpr float kControlsTitleMarginTop = 20.0f;
  static constexpr float kRebindStatusFontSize = 18.0f;
  static constexpr engine::render::Color kRebindStatusColor =
      engine::render::Color::FromBytes(200, 200, 200);
  static constexpr float kVolumeRowSpacing = 15.0f;
  static constexpr float kVolumeLabelFontSize = 24.0f;
  static constexpr float kVolumeLabelWidth = 200.0f;
  static constexpr float kVolumeValueWidth = 80.0f;
  static constexpr float kVolumeButtonSize = 40.0f;
  static constexpr float kBindingColumnSpacing = 10.0f;
  static constexpr float kBindingColumnMarginTop = 10.0f;
  static constexpr float kBindingRowSpacing = 15.0f;
  static constexpr float kBindingLabelFontSize = 22.0f;
  static constexpr float kBindingLabelWidth = 150.0f;
  static constexpr float kBindingButtonWidth = 280.0f;
  static constexpr float kBindingButtonHeight = 45.0f;
  static constexpr float kFullscreenButtonWidth = 400.0f;
  static constexpr float kFullscreenButtonHeight = 50.0f;
  static constexpr float kFullscreenMarginTop = 20.0f;
  static constexpr float kBackButtonWidth = 400.0f;
  static constexpr float kBackButtonHeight = 50.0f;
  static constexpr float kBackMarginTop = 20.0f;
  static constexpr float kVolumeStep = 0.1f;
};

/**
 * @brief Lobby layout values
 */
struct Lobby {
  static constexpr float kFieldHeight = 52.0f;
  static constexpr float kButtonHeight = 56.0f;
  static constexpr float kRoomButtonHeight = 64.0f;
  static constexpr float kRefreshButtonWidth = 140.0f;
  static constexpr float kCreateButtonSize = 56.0f;
  static constexpr float kModalActionButtonWidth = 160.0f;
  static constexpr float kModalMaxPlayersWidth = 180.0f;
  static constexpr float kPanelMargin = 32.0f;
  static constexpr float kListTop = 220.0f;
  static constexpr float kListTitleOffset = 44.0f;
  static constexpr float kListStatusOffset = 14.0f;
  static constexpr float kListTitleFontSize = 28.0f;
  static constexpr float kListStatusFontSize = 18.0f;
  static constexpr float kBannerOffsetX = 52.0f;
  static constexpr float kBannerOffsetBottom = 50.0f;
  static constexpr float kBannerFontSize = 18.0f;
  static constexpr float kControlsY = 110.0f;
  static constexpr float kLabelOffsetY = 20.0f;
  static constexpr float kHostFieldWidth = 260.0f;
  static constexpr float kPortFieldWidth = 100.0f;
  static constexpr float kNameFieldWidth = 260.0f;
  static constexpr float kHostPortSpacing = 10.0f;
  static constexpr float kPortNameSpacing = 20.0f;
  static constexpr float kRefreshCreateSpacing = 12.0f;
  static constexpr float kRoomListTopPadding = 20.0f;
  static constexpr float kRoomListSpacing = 10.0f;
  static constexpr float kModalPaddingX = 24.0f;
  static constexpr float kModalTitleOffsetY = 34.0f;
  static constexpr float kModalTitleFontSize = 28.0f;
  static constexpr float kModalLabelFontSize = 16.0f;
  static constexpr float kModalSubtitleFontSize = 18.0f;
  static constexpr float kModalLabelRow1Y = 84.0f;
  static constexpr float kModalLabelRow2Y = 180.0f;
  static constexpr float kModalPrivateLabelY = 272.0f;
  static constexpr float kModalLabelValueX = 240.0f;
  static constexpr float kModalRoomNameInputY = 102.0f;
  static constexpr float kModalMaxPlayersInputY = 198.0f;
  static constexpr float kModalPrivacyButtonX = 220.0f;
  static constexpr float kModalPrivacyButtonOffsetY = 4.0f;
  static constexpr float kModalPasswordInputY = 292.0f;
  static constexpr float kModalPasswordJoinInputY = 128.0f;
  static constexpr float kModalPrimaryButtonY = 420.0f;
  static constexpr float kModalJoinButtonY = 220.0f;
  static constexpr float kModalWidth = 560.0f;
  static constexpr float kModalHeight = 500.0f;
  static constexpr auto kBannerDuration = std::chrono::seconds(6);
  static constexpr engine::render::Color kPanelColor =
      engine::render::Color::FromBytes(18, 24, 40, 230);
  static constexpr engine::render::Color kAccentColor =
      engine::render::Color::FromBytes(140, 186, 255, 255);
  static constexpr engine::render::Color kMutedTextColor =
      engine::render::Color::FromBytes(180, 190, 210, 255);
  static constexpr engine::render::Color kSoftTextColor =
      engine::render::Color::FromBytes(200, 210, 230, 255);
  static constexpr engine::render::Color kOverlayColor =
      engine::render::Color::FromBytes(0, 0, 0, 180);
  static constexpr std::size_t kGoldenHashRatio = 0x9e3779b9;
  static constexpr std::size_t kHashPrivateSalt = 0xabcddcba;
  static constexpr std::size_t kHashPublicSeed = 0x12344321;
};

/**
 * @brief Pause menu layout values
 */
struct Pause {
  static constexpr float kButtonWidth = 360.0f;
  static constexpr float kButtonHeight = 64.0f;
  static constexpr float kRootPadding = 28.0f;
  static constexpr float kRootSpacing = 14.0f;
  static constexpr float kButtonColumnSpacing = 10.0f;
  static constexpr float kButtonSlotPadding = 8.0f;
  static constexpr float kTitleFontScale = 0.06f;
  static constexpr engine::render::Color kOverlayColor =
      engine::render::Color::FromBytes(6, 10, 22, 210);
};

/**
 * @brief Game over palette values
 */
struct GameOver {
  static constexpr float kRootSpacing = 20.0f;
  static constexpr float kTitleFontScale = 0.08f;
  static constexpr float kScoreFontScale = 0.04f;
  static constexpr float kWaveFontScale = 0.03f;
  static constexpr float kMenuFontScale = 0.03f;
  static constexpr float kSpacerHeight = 40.0f;
  static constexpr engine::render::Color kTitleColor =
      engine::render::Color::FromBytes(255, 50, 50);
  static constexpr engine::render::Color kNormalColor =
      engine::render::Color::White();
  static constexpr engine::render::Color kSelectedColor =
      engine::render::Color::FromBytes(255, 215, 0);
};

}  // namespace client::constants::ui

#endif  // CLIENT_CONSTANTS_UI_CONSTANTS_H_
