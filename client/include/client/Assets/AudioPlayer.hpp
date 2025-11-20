#pragma once

#include <string>

namespace client::assets {

class AudioPlayer {
public:
    void play(const std::string &path);
    void stop();
};

}
