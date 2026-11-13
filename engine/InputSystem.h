#pragma once

#include <algorithm>

#include "ECSEngine.h"
#include "Keyboard.h"

namespace ECSEngine {

const float kMaxRunVelocity = 15;
const float kDashVelocity = 50;
const float kJumpVelocity = 25;
const float kWallJumpVelocity = 25;
const float kGroundRunAcceleration = 5;
const float kAirRunAcceleration = 4;
const float kDashInterval = 60;         // frames between dashes (about one second)
const float kDashDuration = 10;         // frames the dash keeps its high speed
const float kWallSlideVelocity = 5;     // fall-speed cap while pushing into a wall
const float kFastFallAcceleration = 2;  // extra downward acceleration while Down is held
const float kMaxFallVelocity = 30;
const int kWallSlideSoundInterval = 36;

//-----------------------------------------------------------------------
/**
 * @brief Converts keyboard state into player movement and actions.
 *
 * Handles running, dashing, jumping and wall jumps, variable jump height,
 * wall-slide slowdown, and fast-fall. Input sounds are played when the action
 * starts, and frame timers keep the dash cooldown and wall-slide sound from
 * firing too often.
 *
 * Implemented with the help of AI.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("InputSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override { 

        EntityManager<Components...>& entities = engine->GetEntityManager();

        for (auto it = entities.begin(); it != entities.end(); ++it) {

            if (!it->IsValid()) {
                continue;
            }

            const EntityID id = it->GetID();
            if (!entities.template HasComponent<InputComponent>(id)) {
                continue;
            }

            InputComponent& inputComponent = entities.template GetComponent<InputComponent>(id);
            CollisionComponent& collisionComponent = entities.template GetComponent<CollisionComponent>(id);
            MovementComponent& movementComponent = entities.template GetComponent<MovementComponent>(id);

            // TEMP (Travis): Frame-based timers for dash and the wall-slide
            // sound. The engine caps the loop at 60 FPS, so these are roughly
            // seconds * 60. This block can be removed with the notes later.
            if (inputComponent.lastDash > 0) {
                --inputComponent.lastDash;
            }
            if (inputComponent.dashTimer > 0) {
                --inputComponent.dashTimer;
            }
            if (inputComponent.wallSlideSoundTimer > 0) {
                --inputComponent.wallSlideSoundTimer;
            }

            // TEMP (Travis): Edge detection for jump and dash. keydown is this
            // frame; lastKeydown is the state at the start of the frame.
            const bool jumpPressed =
                inputComponent.keydown[Key::Z] && !inputComponent.lastKeydown[Key::Z];
            const bool jumpReleased =
                !inputComponent.keydown[Key::Z] && inputComponent.lastKeydown[Key::Z];

            if (inputComponent.dashTimer <= 0 &&
                inputComponent.keydown[Key::Right] != inputComponent.keydown[Key::Left]) { // If only one of left or right is pressed

                if (inputComponent.keydown[Key::Right]) { // If right

                    if (!collisionComponent.onRightWall) { // If player not against wall

                        if (collisionComponent.onGround) {
                            movementComponent.velocity.x += kGroundRunAcceleration;
                        } else {
                            movementComponent.velocity.x += kAirRunAcceleration;
                        }

                        if (movementComponent.velocity.x > kMaxRunVelocity) { // If above max run velocitt
                            movementComponent.velocity.x = kMaxRunVelocity;
                        }

                    } else if (!collisionComponent.onGround) { // If player against wall and not on ground

                        // Deccelerate fall
                        SlowWallSlide(engine, inputComponent, movementComponent);

                    }

                } else { // If left

                    if (!collisionComponent.onLeftWall) { // If player not against wall

                        if (collisionComponent.onGround) {
                            movementComponent.velocity.x -= kGroundRunAcceleration;
                        } else {
                            movementComponent.velocity.x -= kAirRunAcceleration;
                        }

                        if (movementComponent.velocity.x < -kMaxRunVelocity) { // If above max run velocity
                            movementComponent.velocity.x = -kMaxRunVelocity;
                        }

                    } else if (!collisionComponent.onGround) { // If player against wall and not on ground

                        // Deccelerate fall
                        SlowWallSlide(engine, inputComponent, movementComponent);

                    }
                }

            } else if (inputComponent.dashTimer <= 0) { // If none or both of right and left are pressed
                // Deccelerate entity
                if (movementComponent.velocity.x > 0) {

                    if (collisionComponent.onGround) {
                        movementComponent.velocity.x -= kGroundRunAcceleration;
                    } else {
                        movementComponent.velocity.x -= kAirRunAcceleration;
                    }

                    if (movementComponent.velocity.x < 0) { 
                        movementComponent.velocity.x = 0;
                    }

                } else if (movementComponent.velocity.x < 0) {

                    if (collisionComponent.onGround) {
                        movementComponent.velocity.x += kGroundRunAcceleration;
                    } else {
                        movementComponent.velocity.x += kAirRunAcceleration;
                    }

                    if (movementComponent.velocity.x > 0) { 
                        movementComponent.velocity.x = 0;
                    }
                }

            }

            if (jumpPressed) { // If jump key was just pressed

                if (collisionComponent.onGround) {  // If on ground
                    movementComponent.velocity.y = -kJumpVelocity;
                    PlaySoundIfPresent(engine, "jump");
                } else if (collisionComponent.onRightWall && inputComponent.keydown[Key::Right] &&
                           movementComponent.velocity.y >= 0.0f) { // If pressed against right wall while falling
                    movementComponent.velocity.y = -kWallJumpVelocity;
                    PlaySoundIfPresent(engine, "jump-high");
                } else if (collisionComponent.onLeftWall && inputComponent.keydown[Key::Left] &&
                           movementComponent.velocity.y >= 0.0f) { // If pressed against left wall while falling
                    movementComponent.velocity.y = -kWallJumpVelocity;
                    PlaySoundIfPresent(engine, "jump-high");
                }

                if (movementComponent.velocity.y < -kJumpVelocity) { // If above jump velocity
                    movementComponent.velocity.y = -kJumpVelocity;
                }

            } else if (jumpReleased && movementComponent.velocity.y < 0) { // If jump key is released and player has upward velocity
                // TEMP (Travis): Variable jump height. Releasing early stops
                // the rest of the upward motion, so taps are shorter.
                movementComponent.velocity.y = 0;
            }

            if (inputComponent.keydown[Key::C] && !inputComponent.lastKeydown[Key::C]) {
                // TEMP (Travis): Dash on a fresh C press, with a one-second
                // cooldown. The dash timer keeps the high speed from being
                // clamped by the run-speed code above.
                if (inputComponent.lastDash <= 0) {
                    int dashDirection = 0;
                    if (inputComponent.keydown[Key::Right] != inputComponent.keydown[Key::Left]) {
                        dashDirection = inputComponent.keydown[Key::Right] ? 1 : -1;
                    } else {
                        dashDirection = movementComponent.velocity.x >= 0.0f ? 1 : -1;
                    }

                    movementComponent.velocity.x = dashDirection * kDashVelocity;
                    inputComponent.dashTimer = static_cast<int>(kDashDuration);
                    inputComponent.lastDash = static_cast<int>(kDashInterval);
                }
            }

            if (inputComponent.keydown[Key::Down] && !collisionComponent.onGround) { // If down is pressed and player is in air
                // TEMP (Travis): Fast fall adds downward acceleration and keeps
                // it under a maximum so it stays controllable.
                movementComponent.velocity.y += kFastFallAcceleration;
                movementComponent.velocity.y = std::min(movementComponent.velocity.y, kMaxFallVelocity);
            }
        }

        return kContinue;
    }

private:
    // TEMP (Travis): Wall-slide fall cap and sound. The sound is rate-limited
    // so holding into the wall does not restart it every frame.
    static void SlowWallSlide(EngineView<Components...>* engine, InputComponent& inputComponent,
                              MovementComponent& movementComponent) {
        if (movementComponent.velocity.y <= 0.0f) {
            return;
        }

        movementComponent.velocity.y = std::min(movementComponent.velocity.y, kWallSlideVelocity);

        if (inputComponent.wallSlideSoundTimer <= 0) {
            PlaySoundIfPresent(engine, "footstep-snow");
            inputComponent.wallSlideSoundTimer = kWallSlideSoundInterval;
        }
    }

    // TEMP (Travis): Missing sound assets must not stop the engine tests,
    // which do not load the .ogg files.
    static void PlaySoundIfPresent(EngineView<Components...>* engine, const std::string& name) {
        SoundManager& sounds = engine->GetSoundManager();
        if (sounds.HasSound(name)) {
            sounds.PlaySound(name);
        }
    }
};

}  // namespace ECSEngine
