#pragma once

#include "MathUtil.h"

// Keep legacy includes working; each component is defined in its own header.
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "RespawnComponent.h"

namespace ECSEngine {

// This is a top-left location of an entity all other stored locations are relative to this
// location. This means that moving this single location will move anything else which is relative
// to the location such as:
// - sprite bounding boxes
// - collision bounding boxes
struct LocationComponent {
    Point2D loc;
};

}  // namespace ECSEngine
