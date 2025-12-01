#include "raylib.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  InitWindow(800, 600, "raylib_readable_demo");
  SetTargetFPS(60);

  const char* asset_root = std::getenv("ASSET_ROOT");
  std::string texture_path =
      asset_root ? std::string(asset_root) + "/assets/ship.png"
                 : "assets/ship.png";

  Texture2D tex = LoadTexture(texture_path.c_str());
  if (tex.id == 0) {
    std::cerr << "Failed to load texture: " << texture_path
              << ", using fallback.\n";
    Image img = GenImageColor(32, 32, BLUE);
    tex = LoadTextureFromImage(img);
    UnloadImage(img);
  }

  Vector2 pos{200.f, 200.f};
  Vector2 vel{120.f, 80.f};

  while (!WindowShouldClose()) {
    const float dt = GetFrameTime();
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    if (pos.x > 800) pos.x = 0;
    if (pos.y > 600) pos.y = 0;

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTextureV(tex, pos, WHITE);
    EndDrawing();
  }

  UnloadTexture(tex);
  CloseWindow();
  return 0;
}
