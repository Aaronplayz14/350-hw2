#pragma once

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "Entity.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SpriteComponent.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

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

/// @brief Non-owning entity id of the location the camera follows.
struct CameraFollower {
    EntityID entityID;  // The entity that this camera follows
};

/// @brief Remaining shake frames, initial directional amplitude, and total duration.
struct CameraShake {
    int frameLimit;
    Point2D offset;
    int duration = 14;

    //-------------------------------------------------------------------
    /**
     * @brief Samples directional oscillation with a quadratic fade to zero.
     * @return Temporary display offset; does not advance the timer or move the base camera.
     */
    //-------------------------------------------------------------------
    Point2D GetOffset() const {
        if (frameLimit <= 0 || duration <= 0) return Point2D();
        const float remaining = std::clamp(static_cast<float>(frameLimit) / duration, 0.0F, 1.0F);
        const float phase = static_cast<float>(duration - frameLimit) * 0.95F;
        return offset * (std::cos(phase) * remaining * remaining);
    }
};

}  // namespace ECSEngine
