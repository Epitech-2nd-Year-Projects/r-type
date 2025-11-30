#include <SFML/Graphics.hpp>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

struct Position {
  float x;
  float y;
};

struct Velocity {
  float x;
  float y;
};

struct Config {
  int entities = 500;
  int frames = 180;
};

Config ParseArgs(int argc, char** argv) {
  Config cfg;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--entities" && i + 1 < argc) {
      cfg.entities = std::stoi(argv[++i]);
    } else if (arg == "--frames" && i + 1 < argc) {
      cfg.frames = std::stoi(argv[++i]);
    }
  }
  return cfg;
}

int main(int argc, char** argv) {
  const Config cfg = ParseArgs(argc, argv);

  sf::RenderTexture target;
  if (!target.create(800, 600)) {
    std::cerr << "Failed to create render texture\n";
    return 1;
  }

  sf::Texture tex;
  const char* asset_root = std::getenv("ASSET_ROOT");
  const std::string texture_path =
      asset_root ? std::string(asset_root) + "/assets/ship.png"
                 : "assets/ship.png";
  if (!tex.loadFromFile(texture_path)) {
    std::cerr << "Failed to load texture: " << texture_path
              << ", using fallback.\n";
    sf::Image img;
    img.create(32, 32, sf::Color::Blue);
    if (!tex.loadFromImage(img)) {
      std::cerr << "Failed to create fallback texture\n";
      return 1;
    }
  }

  std::vector<Position> positions(static_cast<size_t>(cfg.entities));
  std::vector<Velocity> velocities(static_cast<size_t>(cfg.entities));

  std::mt19937 rng(42);
  std::uniform_real_distribution<float> dx(-120.f, 120.f);
  std::uniform_real_distribution<float> dy(-80.f, 80.f);

  for (int i = 0; i < cfg.entities; ++i) {
    positions[static_cast<size_t>(i)] =
        Position{static_cast<float>(i % 40) * 20.f,
                 static_cast<float>(i / 40) * 15.f};
    velocities[static_cast<size_t>(i)] = Velocity{dx(rng), dy(rng)};
  }

  const auto start = std::chrono::high_resolution_clock::now();
  const float dt = 1.0f / 60.0f;

  for (int frame = 0; frame < cfg.frames; ++frame) {
    for (int i = 0; i < cfg.entities; ++i) {
      positions[static_cast<size_t>(i)].x += velocities[static_cast<size_t>(i)].x * dt;
      positions[static_cast<size_t>(i)].y += velocities[static_cast<size_t>(i)].y * dt;
      if (positions[static_cast<size_t>(i)].x > 800) positions[static_cast<size_t>(i)].x = 0;
      if (positions[static_cast<size_t>(i)].y > 600) positions[static_cast<size_t>(i)].y = 0;
    }

    target.clear(sf::Color::Black);
    sf::Sprite sprite(tex);
    for (int i = 0; i < cfg.entities; ++i) {
      sprite.setPosition(positions[static_cast<size_t>(i)].x,
                         positions[static_cast<size_t>(i)].y);
      target.draw(sprite);
    }
    target.display();
  }

  const auto end = std::chrono::high_resolution_clock::now();
  const double total_ms =
      std::chrono::duration<double, std::milli>(end - start).count();
  const double per_frame = total_ms / cfg.frames;

  std::cout << "sfml_ecs_bench entities=" << cfg.entities
            << " frames=" << cfg.frames << " total_ms=" << total_ms
            << " per_frame_ms=" << per_frame << "\n";

  return 0;
}
