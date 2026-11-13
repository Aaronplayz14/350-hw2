#pragma once

#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Per-entity keyboard state and short-lived input timers.
 *
 * ECSEngine::ProcessEvents copies keydown into lastKeydown at the start of
 * every frame, then applies the window's key events to keydown. Systems can
 * therefore detect a fresh press by checking keydown[key] && !lastKeydown[key].
 *
 * The timer fields are owned by InputSystem:
 * - lastDash counts down the 60-frame dash cooldown (about two seconds at 30 FPS).
 * - dashTimer keeps the high dash speed alive for a few frames instead of
 *   letting the normal run-speed clamp cancel it immediately.
 * - wallSlideSoundTimer rate-limits the wall-slide sound while the player is
 *   holding into a wall and falling.
 */
//-----------------------------------------------------------------------
struct InputComponent {
    InputComponent(EntityID whichEntity) : entity(whichEntity), lastDash(0), dashTimer(0), wallSlideSoundTimer(0) {}
    EntityID entity;
    int lastDash;
    int dashTimer;            // Frames left of the current dash burst.
    int wallSlideSoundTimer;  // Rate-limits the wall-slide sound.
    std::bitset<sf::Keyboard::ScancodeCount> keydown;
    std::bitset<sf::Keyboard::ScancodeCount> lastKeydown;
};

}  // namespace ECSEngine
