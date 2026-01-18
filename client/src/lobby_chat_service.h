/**
 * @file lobby_chat_service.h
 * @brief Chat message management service for the lobby
 */

#ifndef CLIENT_LOBBY_CHAT_SERVICE_H_
#define CLIENT_LOBBY_CHAT_SERVICE_H_

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "protocol/command.h"

namespace client {

/**
 * @brief Represents a single chat message
 */
struct ChatMessage {
  std::string sender;   ///< Name of the message sender.
  std::string content;  ///< Message content.
  std::string raw;      ///< Full formatted message (sender: content).
  std::optional<std::uint8_t>
      color_index;  ///< Optional color index for sender name.
};

/**
 * @brief Service for managing lobby chat messages
 *
 * Handles sending and receiving chat messages, maintaining a message history
 * buffer, and notifying listeners when new messages arrive.
 */
class LobbyChatService {
 public:
  /// Maximum number of messages to retain in history.
  static constexpr std::size_t kMaxMessageHistory = 50;

  using MessageCallback = std::function<void(const ChatMessage&)>;
  using CommandSender = std::function<bool(const protocol::CommandPayload&)>;

  /**
   * @brief Create a chat service
   * @param command_sender Function to send commands to the server
   */
  explicit LobbyChatService(CommandSender command_sender);

  /**
   * @brief Send a chat message to the server
   * @param message Raw message text (without sender prefix)
   * @return true if the message was queued for sending
   */
  bool SendMessage(std::string_view message);

  /**
   * @brief Handle an incoming chat message from the server
   * @param formatted_message The full "sender: content" message
   */
  void OnChatMessageReceived(std::string_view formatted_message);

  /**
   * @brief Register a callback for new messages
   * @param callback Function to call when a message is received
   */
  void SetMessageCallback(MessageCallback callback);

  /**
   * @brief Access the message history
   * @return Reference to the message history buffer
   */
  const std::deque<ChatMessage>& messages() const { return messages_; }

  /**
   * @brief Clear all message history
   */
  void ClearHistory();

 private:
  CommandSender command_sender_;
  MessageCallback message_callback_;
  std::deque<ChatMessage> messages_;
};

}  // namespace client

#endif  // CLIENT_LOBBY_CHAT_SERVICE_H_
