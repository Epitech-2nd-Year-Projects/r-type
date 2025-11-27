#ifndef PROTOCOL_PROTOCOL_H_
#define PROTOCOL_PROTOCOL_H_

#include "protocol/header.h"
#include "protocol/message_type.h"

/**
 * @file protocol.h
 * @brief Convenience umbrella header for the R-Type UDP protocol library.
 * 
 * Include this header from engine / game code to access the public protocol API.
 * More types (packets, payload structs, channels, etc.) will be added as the
 * implementation progresses.
 */

namespace protocol {
/**
 * @brief Future high-level API declarations will live here, for example:
 * - Encode/decode helpers for full packets.
 * - Client/server channel abstractions.
 * - Factory functions for common messages.
 */
}  // namespace protocol

#endif  // PROTOCOL_PROTOCOL_H_
