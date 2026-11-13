#pragma once

#include <unordered_map>

#include "Entity.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SpriteComponent.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// TEMP (Travis): Extra field notes for the camera pass. These comments are
// intentionally temporary and can be removed once the camera behavior is
// implemented and documented in the usual style.
struct CameraComponent {
    Point2D position;  // [world coordinates] center of the camera view

    // How many world units one screen pixel represents. The platformer uses
    // 1.0 so world coordinates and pixels line up directly.
    float worldUnitsPerPixel;

    // Screen-space exclusion region from the original component design. The
    // current level leaves it empty; it is available if the camera needs an
    // explicit no-go area.
    Rect hardExclusion;

    // Screen-space region where the camera may move. The current level leaves
    // it empty; it can be used for the slow-follow bands described in the
    // project requirements.
    Rect softExclusion;

    // Follow speed. A value of 0 currently snaps; values above 0 are intended
    // for tweening the camera toward the target.
    float speed;
};

struct CameraFollower {
    EntityID entityID;  // The entity that this camera follows
};

struct CameraShake {
    int frameLimit;
    Point2D offset;
};

}  // namespace ECSEngine
