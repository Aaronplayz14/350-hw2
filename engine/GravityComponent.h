#pragma once

#include "MathUtil.h"

namespace ECSEngine {

/// @brief Acceleration added to movement each frame; owns no external resources.
struct GravityComponent {
    GravityComponent(Point2D acceleration, int disabledTimer = 0)
        : acceleration(acceleration), disabledTimer(disabledTimer) {}
    Point2D acceleration;  // Velocity change per simulation frame
    int disabledTimer;     // Reserved field; current GravitySystem does not consume it
};

}  // namespace ECSEngine
