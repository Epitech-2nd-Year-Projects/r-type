#include <array>
#include <atomic>
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>

namespace asio = boost::asio;
using asio::ip::udp;

int main(int argc, char** argv) {
  const unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 4242;
  const unsigned int workers =
      (argc > 2) ? static_cast<unsigned int>(std::stoi(argv[2])) : 1;

  asio::io_context io;
  udp::socket socket(io, udp::endpoint(udp::v4(), port));

  std::cout << "[boost.asio] Echo server listening on " << port << " with "
            << workers << " threads\n";

  std::atomic_bool stop{false};

  auto worker = [&]() {
    std::array<char, 2048> buf{};
    udp::endpoint remote;
    while (!stop.load()) {
      boost::system::error_code ec;
      const size_t n = socket.receive_from(asio::buffer(buf), remote, 0, ec);
      if (ec) break;
      socket.send_to(asio::buffer(buf.data(), n), remote, 0, ec);
      if (ec) break;
    }
  };

  std::vector<std::thread> threads;
  for (unsigned int i = 1; i < workers; ++i) {
    threads.emplace_back(worker);
  }
  worker();
  stop = true;
  for (auto& t : threads) {
    t.join();
  }
  return 0;
}
