#include "protocol/error.h"

namespace protocol {

const char* DecodeErrorToString(DecodeError error) {
  switch (error) {
    case DecodeError::kOk:
      return "ok";
    case DecodeError::kTruncated:
      return "truncated packet";
    case DecodeError::kInvalidVersion:
      return "invalid protocol version";
    case DecodeError::kInvalidMessageType:
      return "invalid message type";
    case DecodeError::kInvalidHeader:
      return "invalid header";
    case DecodeError::kInvalidPayload:
      return "invalid payload";
    case DecodeError::kTooManyEntities:
      return "too many entities";
    case DecodeError::kStringTooLong:
      return "string too long";
  }
  return "unknown error";
}
}  // namespace protocol
