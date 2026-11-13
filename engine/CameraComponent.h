#pragma once

#include "MathUtil.h"

// Keep legacy includes working; follower and shake have dedicated headers.
#include "CameraFollowerComponent.h"
#include "CameraShakeComponent.h"

namespace ECSEngine {

/// @brief Base camera pose and the screen-space bands used by CameraSystem.
struct CameraComponent {
    Point2D position;  // [world coordinates] center of the camera view

    // How many world units one screen pixel represents. The platformer uses
    // 1.0 so world coordinates and pixels line up directly.
    float worldUnitsPerPixel;

    // Screen-space 10%-90% band. Crossing it causes immediate tracking.
    Rect hardExclusion;

    // Screen-space center 40% band. Outside it, the camera eases toward the player.
    Rect softExclusion;

    // Fraction of soft-band distance covered per frame (0.1 in game setup).
    float speed;
};

}  // namespace ECSEngine
