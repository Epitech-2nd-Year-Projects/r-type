#pragma once

#include <functional>
#include <unordered_map>

namespace client::input {

class InputMapper {
public:
    void bind(int key, std::function<void()> action);
    void process();

private:
    std::unordered_map<int, std::function<void()>> _bindings;
};

}
