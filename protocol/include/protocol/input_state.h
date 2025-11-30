#ifndef INPUT_STATE_H_
#define INPUT_STATE_H_

#include <cstdint>
#include "engine/net/packet_buffer.h"

namespace protocol {

  /**
   * @brief Input button flags for player controls.
   */
  enum InputButton : std::uint8_t {
    kInputUp = 1u << 0,      ///< Move up
    kInputDown = 1u << 1,    ///< Move down
    kInputLeft = 1u << 2,    ///< Move left
    kInputRight = 1u << 3,   ///< Move right
    kInputFire = 1u << 4,    ///< Fire weapon
  };

  /**
   * @brief Maximum number of input states to keep in history for redundancy.
   * 
   * Used for input redundancy to ensure reliable delivery over UDP.
   * Recent inputs are resent to handle packet loss.
   */
  inline constexpr std::size_t kMaxInputSequenceHistory = 4;

  /**
   * @brief Single input command from the client.
   * 
   * Represents one frame of player input with timestamp and sequence number.
   */
  struct InputCommand {
    std::uint32_t input_sequence = 0;   ///< Sequence number for this input
    std::uint8_t buttons = 0;           ///< Bitfield of pressed buttons (InputButton flags)
    std::int16_t analog_x = 0;          ///< Analog X-axis value (signed, quantized)
    std::int16_t analog_y = 0;          ///< Analog Y-axis value (signed, quantized)
    std::uint32_t client_time_ms = 0;   ///< Client timestamp in milliseconds
  };

  /**
   * @brief Payload structure for client input state messages.
   * 
   * Contains multiple input commands for redundancy. The most recent commands
   * are sent together to handle packet loss (rolling window approach).
   */
  struct InputStatePayload {
    std::uint8_t command_count = 0;        ///< Number of input commands in this payload (1..kMaxInputSequenceHistory)
    InputCommand commands[kMaxInputSequenceHistory];  ///< Array of input commands
  };

  /**
   * @brief Serializes an InputStatePayload into a PacketBuffer.
   * @param input The input state to serialize.
   * @param writer The packet buffer to write to.
   * @return true on success, false if command_count is invalid (0 or > kMaxInputSequenceHistory).
   */
  bool EncodeInputState(const InputStatePayload& input, engine::net::PacketBuffer& writer);
  
  /**
   * @brief Deserializes an InputStatePayload from a PacketBuffer.
   * @param reader The packet buffer to read from.
   * @param out_input Output parameter for the deserialized input state.
   * @return true on success, false if the buffer is too small or invalid.
   */
  bool DecodeInputState(engine::net::PacketBuffer& reader, InputStatePayload& out_input);

  /**
   * @brief Rolling window of recent input commands for redundancy.
   * 
   * Maintains a fixed-size history of the most recent input commands.
   * When sending input to the server, the last N commands are included
   * to handle packet loss (redundancy mechanism).
   * 
   * The window operates as a FIFO queue with a maximum size of kMaxInputSequenceHistory.
   */
  class InputHistoryWindow {
    public:
      /**
       * @brief Constructs an empty input history window.
       */
      InputHistoryWindow();
      
      /**
       * @brief Adds a new input command to the window.
       * @param command The input command to add.
       * 
       * The command is added at the front of the window, pushing older commands back.
       * If the window is full, the oldest command is discarded.
       */
      void Push(const InputCommand& command);

      /**
       * @brief Builds a payload containing all commands in the window.
       * @return InputStatePayload with all commands currently in the window.
       * 
       * Used to create a packet payload with redundant input history
       * for reliable delivery over UDP.
       */
      InputStatePayload BuildPayload() const;

      /**
       * @brief Returns the current number of commands in the window.
       * @return Number of stored commands (0 to kMaxInputSequenceHistory).
       */
      std::size_t size() const { return count_; }
      
    private:
      InputCommand window_[kMaxInputSequenceHistory]{};  ///< Circular buffer of input commands.
      std::size_t count_{0};                             ///< Current number of commands in window.
  };

}

#endif // !INPUT_STATE_H_
