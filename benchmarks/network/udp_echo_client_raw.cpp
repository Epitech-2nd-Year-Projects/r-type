#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
  const char* host = (argc > 1) ? argv[1] : "127.0.0.1";
  const unsigned short port =
      (argc > 2) ? static_cast<unsigned short>(std::stoi(argv[2])) : 4242;
  const int duration_sec = (argc > 3) ? std::stoi(argv[3]) : 30;
  const size_t payload_size =
      (argc > 4) ? static_cast<size_t>(std::stoi(argv[4])) : 512;

  const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    return 1;
  }

  timeval tv{};
  tv.tv_sec = 0;
  tv.tv_usec = 100'000;
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  sockaddr_in server{};
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  if (::inet_pton(AF_INET, host, &server.sin_addr) != 1) {
    std::cerr << "Invalid host\n";
    ::close(fd);
    return 1;
  }

  std::vector<char> payload(payload_size, 'x');
  std::array<char, 2048> buf{};

  std::uint64_t sent = 0, received = 0;
  const auto start = std::chrono::steady_clock::now();
  const auto deadline = start + std::chrono::seconds(duration_sec);

  while (std::chrono::steady_clock::now() < deadline) {
    const ssize_t n =
        ::sendto(fd, payload.data(), payload.size(), 0,
                 reinterpret_cast<sockaddr*>(&server), sizeof(server));
    if (n < 0) {
      if (errno == EINTR) continue;
      perror("sendto");
      break;
    }
    ++sent;

    sockaddr_in from{};
    socklen_t from_len = sizeof(from);
    const ssize_t r = ::recvfrom(fd, buf.data(), buf.size(), 0,
                                 reinterpret_cast<sockaddr*>(&from), &from_len);
    if (r < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        continue;
      }
      perror("recvfrom");
      break;
    }
    if (static_cast<size_t>(r) == payload_size) {
      ++received;
    }
  }

  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start)
                              .count();
  std::cout << "[raw] Duration " << elapsed_ms << " ms, sent " << sent
            << ", received " << received << ", packets/s "
            << (sent * 1000.0 / elapsed_ms) << "\n";

  ::close(fd);
  return 0;
}
