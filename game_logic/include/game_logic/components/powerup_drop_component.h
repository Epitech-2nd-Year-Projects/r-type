#ifndef GAME_LOGIC_COMPONENTS_POWERUP_DROP_COMPONENT_H_
#define GAME_LOGIC_COMPONENTS_POWERUP_DROP_COMPONENT_H_

namespace game_logic::components {

/**
 * @brief Tag component for entities that drop powerups
 *
 * @details
 * Entities with this component will spawn a random powerup
 * upon death, based on global powerup configuration probabilities.
 */
struct DropsPowerupComponent {
  // Empty tag component
};

}  // namespace game_logic::components

#endif  // GAME_LOGIC_COMPONENTS_POWERUP_DROP_COMPONENT_H_
