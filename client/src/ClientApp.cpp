#include "client/ClientApp.hpp"

#include <raylib.h>
#include <utility>

namespace client {

ClientApp::ClientApp(ClientConfig config)
    : _config(std::move(config)),
      _window(_config.width, _config.height, _config.title) {}

void ClientApp::run() {
    while (!_window.shouldClose()) {
        BeginDrawing();
        _window.clear(RAYWHITE);
        DrawText("Client", 80, 200, 20, BLACK);
        EndDrawing();
    }
}

}
