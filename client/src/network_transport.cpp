#include "network_transport.h"

#include <algorithm>
#include <cctype>
#include <system_error>

#include "engine/time/monotonic_time.h"

namespace client {

std::error_code NetworkTransport::Start(
    std::string_view host, std::uint16_t port,
    std::optional<std::uint16_t> local_port) {
#ifdef RTYPE_TESTING
  test_state_.reset();
#endif
  std::error_code resolve_error;
  auto server = engine::net::Endpoint::FromAddress(host, port, resolve_error);
  if (resolve_error) {
    const bool looks_numeric =
        std::all_of(host.begin(), host.end(), [](unsigned char c) {
          return std::isdigit(c) != 0 || c == '.' || c == ':';
        });
    if (looks_numeric) {
      return resolve_error;
    }
    resolve_error.clear();
    server = engine::net::Endpoint::Resolve(host, port, resolve_error);
  }
  if (resolve_error) return resolve_error;
  if (!server.valid()) return std::make_error_code(std::errc::invalid_argument);

#ifdef RTYPE_TESTING
  test_state_ = TestState{server, true};
  last_receive_ms_.store(engine::time::NowMilliseconds(),
                         std::memory_order_release);
  return {};
#endif

  std::optional<engine::net::Endpoint> bind_endpoint;
  if (local_port.has_value()) {
    bind_endpoint = server.is_ipv6()
                        ? engine::net::Endpoint::AnyIpv6(*local_port)
                        : engine::net::Endpoint::AnyIpv4(*local_port);
  }

  const auto error = client_.Start(server, bind_endpoint);
  if (!error) {
    last_receive_ms_.store(engine::time::NowMilliseconds(),
                           std::memory_order_release);
  }
  return error;
}

void NetworkTransport::Stop() {
#ifdef RTYPE_TESTING
  test_state_.reset();
#endif
  client_.Stop();
  last_receive_ms_.store(0, std::memory_order_release);
}

bool NetworkTransport::Send(engine::net::PacketBuffer packet) {
#ifdef RTYPE_TESTING
  if (test_state_.has_value()) {
    return test_state_->running;
  }
#endif
  return client_.Enqueue(std::move(packet));
}

bool NetworkTransport::Receive(
    engine::net::Client::ReceivedPacket& out_packet) {
#ifdef RTYPE_TESTING
  if (test_state_.has_value()) {
    return false;
  }
#endif
  const bool dequeued = client_.TryDequeue(out_packet);
  if (dequeued) {
    last_receive_ms_.store(engine::time::NowMilliseconds(),
                           std::memory_order_release);
  }
  return dequeued;
}

}  // namespace client
