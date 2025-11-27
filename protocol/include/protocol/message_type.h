#ifndef PROTOCOL_MESSAGE_TYPE_H_
#define PROTOCOL_MESSAGE_TYPE_H_

#include <cstdint>

namespace protocol::message_type {

enum class MessageType : std::uint8_t {
  kInvalid = 0,

  kHello = 1,
  kJoinRequest = 2,
  kJoinAccept = 3,
  kJoinReject = 4,

  kInputState = 5,
  kWorldSnapshot = 6,
  kSpawnEntity = 7,
  kDestroyEntity = 8,
  kPlayerDied = 9,
  kClientCommand = 10,
  kServerCommand = 11,

  kPing = 12,
  kPong = 13
};

}

#endif /* !PROTOCOL_MESSAGE_TYPE_H_ */
