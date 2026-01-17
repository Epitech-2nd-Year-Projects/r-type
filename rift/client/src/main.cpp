#include <iostream>

#include "application.h"
#include "rift_config.h"

int main(int argc, char** argv) {
  rift::client::RiftConfig config = rift::client::LoadRiftConfig(argc, argv);

  std::cout << "Rift Client" << std::endl;
  std::cout << "Connecting to " << config.host << ":" << config.port
            << std::endl;

  rift::client::Application app(config);
  return app.Run();
}
