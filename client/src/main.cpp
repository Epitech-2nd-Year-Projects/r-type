#include "application.h"
#include "client_config.h"

int main(int /*argc*/, char** /*argv*/) {
  const client::ClientConfig config = client::LoadClientConfig();
  client::Application app(config);
  return app.Run();
}
