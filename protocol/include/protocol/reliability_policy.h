#ifndef PROTOCOL_RELIABILITY_POLICY_H_
#define PROTOCOL_RELIABILITY_POLICY_H_

#include <cstdint>

#include "protocol/message_type.h"

namespace protocol {

/**
 * @brief Reliability and connection model for a given message type.
 * 
 * Defines how a message should be handled in terms of delivery guarantees
 * and session requirements.
 */
struct MessageReliabilityPolicy {
  bool reliable;       ///< true if sender should track ACKs and retransmit; false for fire-and-forget.
  bool connectionless; ///< true if can be sent/received without session; false if session required.
};

/**
 * @brief Returns the reliability policy associated with a message type.
 * @param type The message type to query.
 * @return The reliability policy for the given message type.
 */
MessageReliabilityPolicy GetReliabilityPolicy(
    message_type::MessageType type);

/**
 * @brief Convenience helper: true if this type must be sent reliably.
 * @param type The message type to query.
 * @return true if the message type requires reliable delivery, false otherwise.
 */
bool IsReliable(message_type::MessageType type);

/**
 * @brief Convenience helper: true if this type is connectionless.
 * @param type The message type to query.
 * @return true if the message type is connectionless, false otherwise.
 */
bool IsConnectionless(message_type::MessageType type);

}  // namespace protocol

#endif  // PROTOCOL_RELIABILITY_POLICY_H_
