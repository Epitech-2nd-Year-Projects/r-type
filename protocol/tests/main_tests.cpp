#include <iostream>

int RunProtocolSerializationTests();
int RunProtocolReliabilityTests();

int main() {
  int failures = 0;

  failures += RunProtocolSerializationTests() != 0 ? 1 : 0;
  failures += RunProtocolReliabilityTests() != 0 ? 1 : 0;

  if (failures == 0) {
    std::cout << "All protocol tests passed.\n";
  } else {
    std::cout << failures << " test suite(s) failed.\n";
  }

  return failures == 0 ? 0 : 1;
}
