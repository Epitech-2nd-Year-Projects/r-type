#ifndef CLIENT_DEBUG_INSPECTOR_REGISTRATION_H_
#define CLIENT_DEBUG_INSPECTOR_REGISTRATION_H_

#include <functional>

#include "protocol/command.h"

namespace engine::debug {
class ComponentInspectorRegistry;
}

namespace client::debug {

/**
 * @brief Register client-side component inspectors (Sprite, etc.)
 * @param registry The registry to populate
 */
void RegisterClientInspectors(
    engine::debug::ComponentInspectorRegistry& registry, bool is_debug,
    std::function<void(const protocol::CommandPayload&)> send_command);

}  // namespace client::debug

#endif  // CLIENT_DEBUG_INSPECTOR_REGISTRATION_H_
