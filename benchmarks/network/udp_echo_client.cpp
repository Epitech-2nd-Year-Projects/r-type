#include <asio.hpp>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  const char* host = (argc > 1) ? argv[1] : "127.0.0.1";
  const unsigned short port =
      (argc > 2) ? static_cast<unsigned short>(std::stoi(argv[2])) : 4242;
  const int duration_sec = (argc > 3) ? std::stoi(argv[3]) : 30;
  const size_t payload_size =
      (argc > 4) ? static_cast<size_t>(std::stoi(argv[4])) : 512;

  asio::io_context io;
  asio::ip::udp::endpoint endpoint(asio::ip::make_address(host), port);
  asio::ip::udp::socket socket(io);
  socket.open(asio::ip::udp::v4());

  std::vector<char> payload(payload_size, 'x');
  std::uint64_t sent = 0, received = 0;
  auto start = std::chrono::steady_clock::now();
  auto deadline = start + std::chrono::seconds(duration_sec);

  std::array<char, 2048> recv_buf{};
  asio::ip::udp::endpoint remote;

  while (std::chrono::steady_clock::now() < deadline) {
    socket.send_to(asio::buffer(payload), endpoint);
    ++sent;
    std::error_code ec;
    size_t n = socket.receive_from(asio::buffer(recv_buf), remote, 0, ec);
    if (!ec && n == payload_size) ++received;
  }

  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now() - start)
                     .count();
  std::cout << "Duration " << elapsed << " ms, sent " << sent << ", received "
            << received << ", packets/s " << (sent * 1000.0 / elapsed) << "\n";
  return 0;
}
