#ifndef PROTOCOL_COMPRESSION_H_
#define PROTOCOL_COMPRESSION_H_

#include <cstdint>
#include <span>
#include <vector>

namespace protocol {

/**
 * @brief Provides stateless compression and decompression services using LZ4.
 */
class CompressionService {
 public:
  /**
   * @brief Compresses the input data.
   * 
   * @param input Raw data to compress.
   * @param output Buffer to store the compressed data. It will be resized.
   * @return true if compression succeeded, false otherwise.
   * 
   * @details
   * The compressed format includes a 4-byte header containing the 
   * uncompressed size (little-endian uint32_t), followed by the LZ4 stream.
   */
  static bool Compress(std::span<const std::uint8_t> input,
                       std::vector<std::uint8_t>& output);

  /**
   * @brief Decompresses the data.
   * 
   * @param input Compressed data (including the size header).
   * @param output Buffer to store the decompressed data. It will be resized.
   * @return true if decompression succeeded, false otherwise.
   */
  static bool Decompress(std::span<const std::uint8_t> input,
                         std::vector<std::uint8_t>& output);

  /**
   * @brief Returns the maximum compressed size for a given input size (LZ4 bound).
   */
  static int GetMaxCompressedSize(int input_size);
};

}  // namespace protocol

#endif  // PROTOCOL_COMPRESSION_H_
