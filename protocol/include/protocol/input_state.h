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
    kInputFire = 1u << 4     ///< Fire weapon
  };

  /**
   * @brief Payload structure for client input state messages.
   * 
   * Contains the player's input data to be sent from client to server.
   */
  struct InputStatePlayload {
    std::uint32_t input_sequence = 0;   ///< Sequence number for this input
    std::uint8_t buttons = 0;           ///< Bitfield of pressed buttons (InputButton flags)
    std::uint16_t analog_x = 0;         ///< Analog X-axis value (0-65535)
    std::uint16_t analog_y = 0;         ///< Analog Y-axis value (0-65535)
    std::uint32_t timestamp_ms = 0;     ///< Client timestamp in milliseconds
  };

  /**
   * @brief Serializes an InputStatePlayload into a PacketBuffer.
   * @param input The input state to serialize.
   * @param writer The packet buffer to write to.
   */
  void EncodeInputState(const InputStatePlayload& input, engine::net::PacketBuffer& writer);
  
  /**
   * @brief Deserializes an InputStatePlayload from a PacketBuffer.
   * @param reader The packet buffer to read from.
   * @param out_input Output parameter for the deserialized input state.
   * @return true on success, false if the buffer is too small or invalid.
   */
  bool DecodeInputState(engine::net::PacketBuffer& reader, InputStatePlayload& out_input);

}

#endif // !INPUT_STATE_H_
