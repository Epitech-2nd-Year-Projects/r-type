#include "application.h"

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  client::Application app;
  return app.Run();
}
