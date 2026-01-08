#ifndef ENGINE_NET_EVENTS_H_
#define ENGINE_NET_EVENTS_H_

#include "engine/net/received_packet.h"

namespace engine::net {

struct PacketReceivedEvent {
  ReceivedPacket packet;
};

}  // namespace engine::net

#endif  // ENGINE_NET_EVENTS_H_
