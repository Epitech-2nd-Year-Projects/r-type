#include "protocol/reliability_policy.h"

namespace protocol {
namespace {

constexpr MessageReliabilityPolicy MakePolicy(bool reliable,
                                              bool connectionless) {
  return MessageReliabilityPolicy{reliable, connectionless};
}

}  // namespace

MessageReliabilityPolicy GetReliabilityPolicy(message_type::MessageType type) {
  using message_type::MessageType;

  switch (type) {
    case MessageType::kInvalid:
      return MakePolicy(false, true);

    // ----- Connection / session -----
    case MessageType::kHello:
      return MakePolicy(false, true);

    case MessageType::kJoinRequest:
      return MakePolicy(true, true);

    case MessageType::kJoinAccept:
    case MessageType::kJoinReject:
      return MakePolicy(true, true);

    // ----- Gameplay -----
    case MessageType::kInputState:
      return MakePolicy(false, false);

    case MessageType::kWorldSnapshot:
      return MakePolicy(false, false);

    case MessageType::kSpawnEntity:
    case MessageType::kDestroyEntity:
    case MessageType::kPlayerDied:
      return MakePolicy(true, false);

    // ----- Generic commands / events -----
    case MessageType::kClientCommand:
    case MessageType::kServerCommand:
      return MakePolicy(true, false);

    case MessageType::kPing:
    case MessageType::kPong:
      return MakePolicy(false, true);
  }
  return MakePolicy(false, true);
}

bool IsReliable(message_type::MessageType type) {
  return GetReliabilityPolicy(type).reliable;
}

bool IsConnectionless(message_type::MessageType type) {
  return GetReliabilityPolicy(type).connectionless;
}

}  // namespace protocol
