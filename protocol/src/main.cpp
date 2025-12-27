#include "engine/util/logging.h"

int main() {
  engine::util::Logger::Default().Info("[protocol] hello world");
  return 0;
}
