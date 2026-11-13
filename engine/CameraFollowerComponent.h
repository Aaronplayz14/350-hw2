#pragma once

#include "Entity.h"

namespace ECSEngine {

/// @brief Non-owning entity id of the location the camera follows.
struct CameraFollowerComponent {
    EntityID entityID;
};

// Preserve the original engine name for existing scenes and systems.
using CameraFollower = CameraFollowerComponent;

}  // namespace ECSEngine
