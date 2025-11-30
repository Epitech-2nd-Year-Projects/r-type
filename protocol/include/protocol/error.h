#ifndef PROTOCOL_ERROR_H_
#define PROTOCOL_ERROR_H_

#include <cstdint>

namespace protocol {

/**
 * @brief Error codes for protocol packet decoding operations.
 * 
 * Used to provide detailed information about why a packet decode operation failed.
 */
enum class DecodeError : std::uint8_t {
  kOk = 0,  ///< No error, operation succeeded.

  // Generic framing / length errors
  kTruncated,              ///< Not enough bytes in the buffer.
  kInvalidVersion,         ///< Header version does not match kProtocolVersion.
  kInvalidMessageType,     ///< Header message type is unknown or unsupported.
  kInvalidHeader,          ///< Header fields are inconsistent or malformed.
  kInvalidPayload,         ///< Payload decoding failed (generic error).

  // Specific payload validation errors
  kTooManyEntities,        ///< Snapshot entity count exceeds maximum limit.
  kStringTooLong,          ///< String length exceeds protocol maximum.
};

/**
 * @brief Converts a DecodeError to a human-readable string.
 * @param error The error code to convert.
 * @return A null-terminated string describing the error.
 */
const char* DecodeErrorToString(DecodeError error);

}  // namespace protocol

#endif  // PROTOCOL_ERROR_H_
