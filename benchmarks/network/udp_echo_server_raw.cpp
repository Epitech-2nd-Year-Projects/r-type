#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

namespace {

std::atomic_bool g_stop{false};

void SignalHandler(int) { g_stop.store(true); }

int MakeSocket(unsigned short port) {
  const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    return -1;
  }
  int reuse = 1;
  ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    perror("bind");
    ::close(fd);
    return -1;
  }
  ::fcntl(fd, F_SETFL, O_NONBLOCK);
  return fd;
}

void Worker(int fd) {
  std::array<char, 2048> buf{};
  sockaddr_in remote{};
  socklen_t remote_len = sizeof(remote);
  while (!g_stop.load()) {
    remote_len = sizeof(remote);
    const ssize_t n =
        ::recvfrom(fd, buf.data(), buf.size(), 0,
                   reinterpret_cast<sockaddr*>(&remote), &remote_len);
    if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        std::this_thread::yield();
        continue;
      }
      perror("recvfrom");
      break;
    }
    if (n == 0) continue;
    const ssize_t sent =
        ::sendto(fd, buf.data(), static_cast<size_t>(n), 0,
                 reinterpret_cast<sockaddr*>(&remote), remote_len);
    if (sent < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        continue;
      }
      perror("sendto");
      break;
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  const unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 4242;
  const unsigned int workers =
      (argc > 2) ? static_cast<unsigned int>(std::stoi(argv[2])) : 1;

  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  const int fd = MakeSocket(port);
  if (fd < 0) {
    return 1;
  }

  std::cout << "[raw] Echo server listening on " << port << " with " << workers
            << " threads\n";

  std::vector<std::thread> threads;
  threads.reserve(workers > 0 ? workers - 1 : 0);
  for (unsigned int i = 1; i < workers; ++i) {
    threads.emplace_back(Worker, fd);
  }
  Worker(fd);

  for (auto& t : threads) {
    t.join();
  }
  ::close(fd);
  return 0;
}
