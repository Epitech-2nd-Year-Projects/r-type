#include "protocol/error.h"

#include <string_view>

namespace protocol {

std::string_view DecodeErrorToString(DecodeError error) {
  switch (error) {
    case DecodeError::kOk:
      return "ok";
    case DecodeError::kUnexpectedEndOfBuffer:
      return "unexpected end of buffer";
    case DecodeError::kInvalidHeader:
      return "invalid header";
    case DecodeError::kUnknownMessageType:
      return "unknown message type";
    case DecodeError::kVersionMismatch:
      return "version mismatch";
    case DecodeError::kInvalidPayload:
      return "invalid payload";
    case DecodeError::kInvalidSnapshotId:
      return "invalid snapshot id";
  }
  return "unknown error";
}

void UpdateDecodeMetrics(DecodeMetrics& metrics, DecodeError error) {
  ++metrics.total_packets;

  if (error == DecodeError::kOk) {
    return;
  }

  ++metrics.rejected_packets;

  switch (error) {
    case DecodeError::kUnexpectedEndOfBuffer:
      ++metrics.unexpected_end_of_buffer;
      break;
    case DecodeError::kInvalidHeader:
      ++metrics.invalid_header;
      break;
    case DecodeError::kUnknownMessageType:
      ++metrics.unknown_message_type;
      break;
    case DecodeError::kVersionMismatch:
      ++metrics.version_mismatch;
      break;
    case DecodeError::kInvalidPayload:
      ++metrics.invalid_payload;
      break;
    case DecodeError::kInvalidSnapshotId:
      ++metrics.invalid_snapshot_id;
      break;
    case DecodeError::kOk:
      break;
  }
}

}  // namespace protocol
