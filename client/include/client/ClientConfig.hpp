#pragma once

#include <string>

namespace client {

struct ClientConfig {
    int width{800};
    int height{600};
    std::string title{"Client"};
};

}
