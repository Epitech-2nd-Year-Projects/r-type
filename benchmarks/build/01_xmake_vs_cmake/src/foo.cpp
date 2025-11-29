#include "foo.h"

int sum(const std::vector<int> &values) {
  int total = 0;
  for (int v : values) total += v;
  return total;
}

std::string greet(std::string name, int times) {
  std::string out;
  out.reserve(name.size() * static_cast<size_t>(times));
  for (int i = 0; i < times; ++i) out += name;
  return out;
}
