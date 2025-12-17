#include <asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main(int argc, char** argv) {
  const unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 4242;
  const unsigned int workers =
      (argc > 2) ? static_cast<unsigned int>(std::stoi(argv[2])) : 1;

  asio::io_context io;
  asio::ip::udp::socket socket(
      io, asio::ip::udp::endpoint(asio::ip::udp::v4(), port));
  std::cout << "Echo server listening on " << port << " with " << workers
            << " threads\n";

  auto worker = [&]() {
    std::array<char, 2048> buf{};
    asio::ip::udp::endpoint remote;
    for (;;) {
      std::error_code ec;
      size_t n = socket.receive_from(asio::buffer(buf), remote, 0, ec);
      if (ec) break;
      socket.send_to(asio::buffer(buf.data(), n), remote, 0, ec);
      if (ec) break;
    }
  };

  std::vector<std::thread> threads;
  for (unsigned int i = 1; i < workers; ++i) threads.emplace_back(worker);
  worker();
  for (auto& t : threads) t.join();
  return 0;
}
