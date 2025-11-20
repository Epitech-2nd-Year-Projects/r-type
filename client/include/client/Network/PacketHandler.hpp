#pragma once

#include <cstdint>
#include <vector>

namespace client::network {

class PacketHandler {
public:
    void handle(const std::vector<std::uint8_t> &payload);
};

}
