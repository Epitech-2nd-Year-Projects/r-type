/**
 * @file lobby_chat_service.cpp
 * @brief Implementation of lobby chat service
 */

#include "lobby_chat_service.h"

#include <utility>

#include "protocol/chat.h"
#include "protocol/command.h"

namespace client {

LobbyChatService::LobbyChatService(CommandSender command_sender)
    : command_sender_(std::move(command_sender)) {}

bool LobbyChatService::SendMessage(std::string_view message) {
  if (message.empty() || message.size() > protocol::kMaxChatMessageLength) {
    return false;
  }
  if (!command_sender_) {
    return false;
  }

  protocol::CommandPayload payload{};
  payload.command_id =
      static_cast<std::uint16_t>(protocol::CommandType::kChatMessage);
  payload.payload.assign(message.begin(), message.end());

  return command_sender_(payload);
}

void LobbyChatService::OnChatMessageReceived(
    std::string_view formatted_message) {
  ChatMessage msg{};
  msg.raw = std::string(formatted_message);

  std::string_view sender;
  std::string_view content;
  if (protocol::ParseChatMessage(formatted_message, sender, content)) {
    msg.sender = std::string(sender);

    if (!content.empty() && static_cast<unsigned char>(content[0]) < 32) {
      msg.color_index = static_cast<std::uint8_t>(content[0]);
      msg.content = std::string(content.substr(1));
    } else {
      msg.content = std::string(content);
    }
  } else {
    msg.sender.clear();
    msg.content = msg.raw;
  }

  messages_.push_back(std::move(msg));

  while (messages_.size() > kMaxMessageHistory) {
    messages_.pop_front();
  }

  if (message_callback_) {
    message_callback_(messages_.back());
  }
}

void LobbyChatService::SetMessageCallback(MessageCallback callback) {
  message_callback_ = std::move(callback);
}

void LobbyChatService::ClearHistory() { messages_.clear(); }

}  // namespace client
