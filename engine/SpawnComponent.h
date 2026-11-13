#pragma once

#include <unordered_map>

#include "MathUtil.h"
#include "Entity.h"
#include "SpriteManager.h"

namespace ECSEngine {

/// @brief Frame countdown and remaining spawn budget for one spawner tile.
/// Entity and sprite ids are non-owning handles. SpawnSystem currently supports
/// only spawnType == "star" and requires the tile's LocationComponent.
class SpawnComponent {
public:
    EntityID entityID;  // The entity that this spawn component is attached to
    std::string spawnType;
    SpriteID spawnTextureID;
    int timeToNextSpawn;
    int timeBetweenSpawns;  // in frames
    int totalSpawns;

private:
};

}  // namespace ECSEngine
