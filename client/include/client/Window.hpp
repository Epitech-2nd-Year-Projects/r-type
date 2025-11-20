#pragma once

#include <string>

#include <raylib.h>

namespace client {

class Window {
public:
    Window(int width, int height, const std::string &title) {
        InitWindow(width, height, title.c_str());
        SetTargetFPS(60);
    }

    ~Window() {
        if (IsWindowReady()) {
            CloseWindow();
        }
    }

    bool shouldClose() const {
        return WindowShouldClose();
    }

    void clear(Color color) const {
        ClearBackground(color);
    }
};

}
