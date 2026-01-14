#include "rift/systems/fighter_input_system.h"

#include "engine/ecs/components/position_component.h"
#include "engine/ecs/components/velocity_component.h"
#include "engine/ecs/indexed_zipper.h"
#include "rift/components/fighter_component.h"
#include "rift/game_instance.h"

namespace rift::systems {

FighterInputSystem::FighterInputSystem(GameInstance& instance)
    : instance_(instance) {}

void FighterInputSystem::Update(engine::ecs::Registry& registry,
                                engine::time::TimeDelta) {
  // TODO: Maybe implement this system for input buffering.
}

}  // namespace rift::systems
