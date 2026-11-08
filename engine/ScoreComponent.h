#pragma once

#include <unordered_map>

#include "MathUtil.h"
#include "SpriteManager.h"

namespace ECSEngine {

// The sprite rectangle is relative to the entity location in the world
// This is only defined if an entity also has a LocationComponent.
class ScoreComponent {
public:
    int score;
    SpriteID digitTextureID[10];
    EntityID digitID[3];

private:
};

}  // namespace ECSEngine
