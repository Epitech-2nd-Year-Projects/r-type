#pragma once

#include <string>

namespace client::network {

class UdpClient {
public:
    void connect(const std::string &host, unsigned short port);
    void send(const std::string &payload);
    void poll();
};

}
