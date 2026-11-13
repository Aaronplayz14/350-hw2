#pragma once

#include "MathUtil.h"

namespace ECSEngine {

/// @brief Per-frame displacement applied by MovementSystem to an entity's location.
struct MovementComponent {
    Point2D velocity;  // World units per simulation frame (the game targets 30 FPS)
};

}  // namespace ECSEngine
