#include <SFML/Graphics.hpp>

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  sf::RenderWindow window(sf::VideoMode(800, 600), "sfml_readable_demo");
  window.setFramerateLimit(60);

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

  sf::Sprite sprite(tex);
  sf::Vector2f pos{200.f, 200.f};
  sf::Vector2f vel{120.f, 80.f};

  sf::Clock clock;
  while (window.isOpen()) {
    sf::Event event{};
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) window.close();
    }

    const float dt = clock.restart().asSeconds();
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    if (pos.x > 800) pos.x = 0;
    if (pos.y > 600) pos.y = 0;

    sprite.setPosition(pos);

    window.clear(sf::Color::Black);
    window.draw(sprite);
    window.display();
  }

  return 0;
}
