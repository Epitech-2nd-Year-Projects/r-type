#ifndef CLIENT_DEBUG_INSPECTOR_REGISTRATION_H_
#define CLIENT_DEBUG_INSPECTOR_REGISTRATION_H_

namespace engine::debug {
class ComponentInspectorRegistry;
}

namespace client::debug {

/**
 * @brief Register client-side component inspectors (Sprite, etc.)
 * @param registry The registry to populate
 */
void RegisterClientInspectors(
    engine::debug::ComponentInspectorRegistry& registry);

}  // namespace client::debug

#endif  // CLIENT_DEBUG_INSPECTOR_REGISTRATION_H_
