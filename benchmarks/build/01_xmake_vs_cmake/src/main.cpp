#include <fmt/core.h>
#include <iostream>
#include <string>
#include <vector>

#include "foo.h"

int main() {
  std::vector<int> values;
  values.reserve(1000);
  for (int i = 0; i < 1000; ++i) values.push_back(i % 10);

  const int total = sum(values);
  const std::string text = greet("rtype", 3);

  fmt::print("sum={} greet={}\n", total, text);
  return 0;
}
