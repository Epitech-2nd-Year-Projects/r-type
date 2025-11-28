#include <cstdint>
#include <iostream>

#include "protocol/reliability.h"
#include "protocol/sequence_tracker.h"

namespace {

template <typename F>
bool RunTest(const char* name, F&& fn) {
  const bool ok = fn();
  std::cout << (ok ? "[ OK ] " : "[FAIL] ") << name << "\n";
  return ok;
}

bool TestSequenceMoreRecentSimple() {
  using protocol::IsSequenceMoreRecent;

  if (!IsSequenceMoreRecent(5u, 4u)) return false;
  if (IsSequenceMoreRecent(4u, 5u)) return false;
  if (IsSequenceMoreRecent(10u, 10u)) return false;

  return true;
}

bool TestSequenceMoreRecentWrapAround() {
  using protocol::IsSequenceMoreRecent;
  using protocol::kSequenceHalfRange;

  const std::uint32_t max = 0xFFFFFFFFu;
  const std::uint32_t zero = 0u;

  // 0 est plus récent que max (wrap).
  if (!IsSequenceMoreRecent(zero, max)) return false;
  // max n'est pas plus récent que 0.
  if (IsSequenceMoreRecent(max, zero)) return false;

  // Un paquet un peu avant max est plus vieux que max.
  if (IsSequenceMoreRecent(max - 10u, max)) return false;
  // Et max est plus récent qu'un paquet un peu avant.
  if (!IsSequenceMoreRecent(max, max - 10u)) return false;

  (void)kSequenceHalfRange;  // juste pour éviter un warning si tu ne l'utilises
                             // pas.

  return true;
}

bool TestIsPacketAckedBasics() {
  using protocol::IsPacketAcked;

  const std::uint32_t ack = 100u;
  const std::uint32_t ack_bits = 0b00000000000000000000000000000001u;
  // ack = 100, ack_bits dit que 99 est reçu (bit 0 = 1).

  if (!IsPacketAcked(100u, ack, ack_bits)) return false;  // égal à ack -> acké
  if (!IsPacketAcked(99u, ack, ack_bits)) return false;   // bit 0 dans ack_bits
  if (IsPacketAcked(98u, ack, ack_bits)) return false;  // bit 1 = 0 -> pas acké

  // Un paquet plus récent que ack ne peut pas être acké.
  if (IsPacketAcked(130u, ack, ack_bits)) return false;

  // Un paquet trop ancien (>32) ne peut pas être représenté -> pas acké.
  const std::uint32_t very_old = ack - 40u;
  if (IsPacketAcked(very_old, ack, ack_bits)) return false;

  return true;
}

bool TestSequenceTrackerBasic() {
  protocol::SequenceTracker tracker;
  protocol::Header header{};

  tracker.OnRemoteSequenceReceived(100u);
  tracker.FillAckFields(&header);

  if (header.ack != 100u) {
    std::cout << "Expected ack=100 after first packet, got " << header.ack
              << "\n";
    return false;
  }
  if (header.ack_bits != 0u) {
    std::cout << "Expected ack_bits=0 after first packet, got "
              << header.ack_bits << "\n";
    return false;
  }

  tracker.OnRemoteSequenceReceived(99u);
  tracker.FillAckFields(&header);

  if (header.ack != 100u) {
    std::cout << "Expected ack=100 after second packet, got " << header.ack
              << "\n";
    return false;
  }

  if (header.ack_bits != 1u) {
    std::cout << "Expected ack_bits=1 (bit0 set for seq=99), got "
              << header.ack_bits << "\n";
    return false;
  }

  if (!protocol::IsPacketAcked(100u, header.ack, header.ack_bits)) {
    std::cout << "Packet 100 should be acked\n";
    return false;
  }
  if (!protocol::IsPacketAcked(99u, header.ack, header.ack_bits)) {
    std::cout << "Packet 99 should be acked through ack_bits\n";
    return false;
  }
  if (protocol::IsPacketAcked(98u, header.ack, header.ack_bits)) {
    std::cout << "Packet 98 should NOT be acked (bit1=0)\n";
    return false;
  }

  return true;
}

}  // namespace

int RunProtocolReliabilityTests() {
  int failures = 0;

  if (!RunTest("SequenceMoreRecent simple", &TestSequenceMoreRecentSimple)) {
    ++failures;
  }
  if (!RunTest("SequenceMoreRecent wrap-around",
               &TestSequenceMoreRecentWrapAround)) {
    ++failures;
  }
  if (!RunTest("IsPacketAcked basics", &TestIsPacketAckedBasics)) {
    ++failures;
  }
  if (!RunTest("SequenceTracker basic", &TestSequenceTrackerBasic)) {
    ++failures;
  }

  if (failures == 0) {
    std::cout << "All reliability tests passed.\n";
  } else {
    std::cout << failures << " reliability test(s) failed.\n";
  }

  return failures == 0 ? 0 : 1;
}
