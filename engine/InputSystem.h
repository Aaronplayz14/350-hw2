#pragma once

#include "Keyboard.h"

namespace ECSEngine {

const float kMaxRunVelocity = 15;
const float kDashVelocity = 50;
const float kJumpVelocity = 25;
const float kGroundRunAcceleration = 5;
const float kAirRunAcceleration = 4;
const float kDashInterval = 25;

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

            if (inputComponent.keydown[Key::Right] != inputComponent.keydown[Key::Left]) { // If only one of left or right is pressed

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

                        // TODO: Deccelerate fall

                    }
                }

            } else { // If none or both of right and left are pressed
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

            if (inputComponent.keydown[Key::Z]) { // If jump key is pressed

                if (collisionComponent.onGround) {  // If on ground
                    movementComponent.velocity.y -= kJumpVelocity;
                } else if (collisionComponent.onRightWall && inputComponent.keydown[Key::Right]) { // If pressed against right wall in air
                    movementComponent.velocity.y -= kJumpVelocity;
                } else if (collisionComponent.onLeftWall && inputComponent.keydown[Key::Left]) { // If pressed against left wall in air
                    movementComponent.velocity.y -= kJumpVelocity;
                }

                if (movementComponent.velocity.y < -kJumpVelocity) { // If above jump velocity
                    movementComponent.velocity.y = -kJumpVelocity;
                }

            } else if (movementComponent.velocity.y < 0) { // If jump key is not pressed and player has upward velocity
                // TODO: Deccelerate or halt upward velocity
            }

            if (inputComponent.keydown[Key::C]) {

            }

            if (inputComponent.keydown[Key::Down] && !collisionComponent.onGround){ // If down is pressed and player is in air
                // TODO: Accelerate fall
            }
        }

        return kContinue; }
};

}  // namespace ECSEngine
