/**
 * @file chat.h
 * @brief Chat message constants and utilities for lobby communication
 */

#ifndef PROTOCOL_CHAT_H_
#define PROTOCOL_CHAT_H_

#include <cstddef>
#include <string>
#include <string_view>

namespace protocol {

/// @brief Maximum length for a single chat message (not including sender name).
inline constexpr std::size_t kMaxChatMessageLength = 256;

/// @brief Maximum length for a formatted chat message (sender: message).
inline constexpr std::size_t kMaxFormattedChatLength = 320;

/// @brief Delimiter between sender name and message content.
inline constexpr char kChatMessageDelimiter = ':';

/**
 * @brief Format a chat message with sender name prefix.
 * @param sender_name Name of the message sender.
 * @param message Raw message content.
 * @return Formatted message in "sender: message" format.
 */
inline std::string FormatChatMessage(std::string_view sender_name,
                                     std::string_view message) {
  std::string formatted;
  formatted.reserve(sender_name.size() + 2 + message.size());
  formatted.append(sender_name);
  formatted.push_back(kChatMessageDelimiter);
  formatted.push_back(' ');
  formatted.append(message);
  return formatted;
}

/**
 * @brief Parse a formatted chat message to extract sender and content.
 * @param formatted_message The full "sender: message" string.
 * @param out_sender Output parameter for sender name.
 * @param out_message Output parameter for message content.
 * @return true if parsing succeeded, false if delimiter not found.
 */
inline bool ParseChatMessage(std::string_view formatted_message,
                             std::string_view& out_sender,
                             std::string_view& out_message) {
  const auto delimiter_pos = formatted_message.find(kChatMessageDelimiter);
  if (delimiter_pos == std::string_view::npos) {
    return false;
  }
  out_sender = formatted_message.substr(0, delimiter_pos);
  const std::size_t message_start = delimiter_pos + 1;
  if (message_start < formatted_message.size() &&
      formatted_message[message_start] == ' ') {
    out_message = formatted_message.substr(message_start + 1);
  } else {
    out_message = formatted_message.substr(message_start);
  }
  return true;
}

/**
 * @brief Validate that a chat message meets length requirements.
 * @param message Raw message to validate.
 * @return true if the message is valid (non-empty and within length limit).
 */
inline bool IsValidChatMessage(std::string_view message) {
  return !message.empty() && message.size() <= kMaxChatMessageLength;
}

}  // namespace protocol

#endif  // PROTOCOL_CHAT_H_
