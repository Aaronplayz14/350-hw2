#pragma once

#include <algorithm>
#include <cmath>

#include "MathUtil.h"

namespace ECSEngine {

/// @brief Remaining shake frames, initial directional amplitude, and total duration.
struct CameraShakeComponent {
    int frameLimit;
    Point2D offset;
    int duration = 14;

    //-------------------------------------------------------------------
    /**
     * @brief Samples directional oscillation with a quadratic fade to zero.
     * @return Display offset; does not advance the timer or move the base camera.
     */
    //-------------------------------------------------------------------
    Point2D GetOffset() const {
        if (frameLimit <= 0 || duration <= 0) return Point2D();
        const float remaining = std::clamp(static_cast<float>(frameLimit) / duration, 0.0F, 1.0F);
        const float phase = static_cast<float>(duration - frameLimit) * 0.95F;
        return offset * (std::cos(phase) * remaining * remaining);
    }
};

// Preserve the original engine name for existing scenes and systems.
using CameraShake = CameraShakeComponent;

}  // namespace ECSEngine
