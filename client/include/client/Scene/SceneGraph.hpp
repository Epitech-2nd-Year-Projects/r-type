#pragma once

#include <vector>

namespace client::scene {

class SceneGraph {
public:
    void update(float dt);
    void clear();

private:
    std::vector<int> _nodes;
};

}
