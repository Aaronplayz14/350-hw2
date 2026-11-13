#pragma once

#include "MathUtil.h"

namespace ECSEngine {

/// @brief World-space origin restored when the player touches a damaging tile.
struct RespawnComponent {
    Point2D loc;
};

}  // namespace ECSEngine
