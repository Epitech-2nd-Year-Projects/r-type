#include <array>
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <vector>

namespace asio = boost::asio;
using asio::ip::udp;

int main(int argc, char** argv) {
  const char* host = (argc > 1) ? argv[1] : "127.0.0.1";
  const unsigned short port =
      (argc > 2) ? static_cast<unsigned short>(std::stoi(argv[2])) : 4242;
  const int duration_sec = (argc > 3) ? std::stoi(argv[3]) : 30;
  const size_t payload_size =
      (argc > 4) ? static_cast<size_t>(std::stoi(argv[4])) : 512;

  asio::io_context io;
  udp::endpoint endpoint(asio::ip::make_address(host), port);
  udp::socket socket(io);
  socket.open(udp::v4());

  std::vector<char> payload(payload_size, 'x');
  std::array<char, 2048> buf{};

  std::uint64_t sent = 0, received = 0;
  const auto start = std::chrono::steady_clock::now();
  const auto deadline = start + std::chrono::seconds(duration_sec);

  while (std::chrono::steady_clock::now() < deadline) {
    socket.send_to(asio::buffer(payload), endpoint);
    ++sent;
    udp::endpoint remote;
    boost::system::error_code ec;
    const size_t n = socket.receive_from(asio::buffer(buf), remote, 0, ec);
    if (!ec && n == payload_size) {
      ++received;
    }
  }

  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start)
                              .count();
  std::cout << "[boost.asio] Duration " << elapsed_ms << " ms, sent " << sent
            << ", received " << received << ", packets/s "
            << (sent * 1000.0 / elapsed_ms) << "\n";
  return 0;
}
