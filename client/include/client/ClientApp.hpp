#pragma once

#include "client/ClientConfig.hpp"
#include "client/Window.hpp"

namespace client {

class ClientApp {
public:
    explicit ClientApp(ClientConfig config);

    void run();

private:
    ClientConfig _config;
    Window _window;
};

}
