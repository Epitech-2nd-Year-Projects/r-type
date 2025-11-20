#include "client/ClientApp.hpp"
#include "client/ClientConfig.hpp"

int main() {
    client::ClientConfig config;
    config.width = 800;
    config.height = 600;
    config.title = "Client";

    client::ClientApp app(config);
    app.run();
    return 0;
}
