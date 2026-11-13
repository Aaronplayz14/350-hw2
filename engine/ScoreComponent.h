#pragma once

#include <unordered_map>

#include "MathUtil.h"
#include "Entity.h"
#include "SpriteManager.h"

namespace ECSEngine {

/// @brief Numeric score and non-owning handles to the registered numeral sprites
/// and three display entities. Game setup creates these resources; ScoreSystem
/// changes their sprite ids and clamps the displayed value to 000-999.
class ScoreComponent {
public:
    int score;
    SpriteID digitTextureID[10];
    EntityID digitID[3];

private:
};

}  // namespace ECSEngine
