#ifndef PROTOCOL_ERROR_H_
#define PROTOCOL_ERROR_H_

#include <cstdint>
#include <string_view>

namespace protocol {

/**
 * @brief Error codes for protocol packet decoding operations.
 * 
 * Error names follow the issue requirements:
 *   InvalidHeader, UnknownMessageType, UnexpectedEndOfBuffer,
 *   InvalidSnapshotId, VersionMismatch.
 */
enum class DecodeError : std::uint8_t {
  kOk = 0,  ///< No error, operation succeeded.

  // Framing / buffer errors
  kUnexpectedEndOfBuffer,   ///< Not enough bytes in the buffer.

  // Header / version errors
  kInvalidHeader,           ///< Header fields inconsistent or malformed.
  kUnknownMessageType,      ///< Header message_type unknown or unsupported.
  kVersionMismatch,         ///< Header version does not match kProtocolVersion.

  // Payload / content errors
  kInvalidPayload,          ///< Payload decoding failed (generic error).

  // Snapshot semantics (for server/client validation)
  kInvalidSnapshotId,       ///< SnapshotId / BaseSnapshotId is invalid.
};

/**
 * @brief Converts a DecodeError to a human-readable string.
 * @param error The error code to convert.
 * @return String describing the error.
 */
std::string_view DecodeErrorToString(DecodeError error);

/**
 * @brief Simple metrics for tracking packet decode errors.
 * 
 * Used on server/client side to count rejected packets,
 * invalid versions, etc.
 */
struct DecodeMetrics {
  std::uint64_t total_packets = 0;            ///< Total number of packets processed.
  std::uint64_t rejected_packets = 0;         ///< Number of packets rejected due to errors.

  std::uint64_t unexpected_end_of_buffer = 0; ///< Count of kUnexpectedEndOfBuffer errors.
  std::uint64_t invalid_header = 0;           ///< Count of kInvalidHeader errors.
  std::uint64_t unknown_message_type = 0;     ///< Count of kUnknownMessageType errors.
  std::uint64_t version_mismatch = 0;         ///< Count of kVersionMismatch errors.
  std::uint64_t invalid_payload = 0;          ///< Count of kInvalidPayload errors.
  std::uint64_t invalid_snapshot_id = 0;      ///< Count of kInvalidSnapshotId errors.
};

/**
 * @brief Updates metrics according to the given decode error.
 * @param metrics Metrics instance to update.
 * @param error Error code returned by DecodePacket (kOk or other).
 * 
 * Should be called after each DecodePacket attempt (whether successful or not).
 */
void UpdateDecodeMetrics(DecodeMetrics& metrics, DecodeError error);

}  // namespace protocol

#endif  // PROTOCOL_ERROR_H_
