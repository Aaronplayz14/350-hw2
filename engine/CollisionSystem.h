#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

#include "ECSEngine.h"

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Resolves static/dynamic collisions and player/star interactions.
 *
 * The pass snapshots and clears the previous frame's collision flags, stops
 * the player against static map tiles, bounces stars off those tiles, swaps
 * the velocity of two colliding stars, and collects a star when the player
 * touches it. Collection adds score and plays the gem sound.
 *
 * Implemented with the help of AI.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        
        EntityManager<Components...>& entities = engine->GetEntityManager();

        // Collision flags are cleared here instead of MovementSystem. Input
        // needs last frame's flags before this system runs, and this system
        // needs them to detect a fresh landing.
        struct PreviousFlags {
            std::uint8_t onGround = 0;
            std::uint8_t onCeiling = 0;
            std::uint8_t onLeftWall = 0;
            std::uint8_t onRightWall = 0;
        };

        const std::size_t entityCount = entities.GetEntityCount();
        std::vector<PreviousFlags> previousFlags(entityCount);
        std::vector<std::uint8_t> pendingStarRemoval(entityCount, 0);

        for (EntityID id = 0; id < entityCount; ++id) {
            if (!entities.IsValid(id) ||
                !entities.template HasComponent<CollisionComponent>(id)) {
                continue;
            }

            CollisionComponent& collisionComponent =
                entities.template GetComponent<CollisionComponent>(id);
            previousFlags[id] = PreviousFlags{collisionComponent.onGround,
                                              collisionComponent.onCeiling,
                                              collisionComponent.onLeftWall,
                                              collisionComponent.onRightWall};

            collisionComponent.onCeiling = 0;
            collisionComponent.onGround = 0;
            collisionComponent.onRightWall = 0;
            collisionComponent.onLeftWall = 0;
        }

        for (auto it1 = entities.begin(); std::next(it1) != entities.end(); ++it1) {

            if (!it1->IsValid()) {
                continue;
            }

            const EntityID id1 = it1->GetID();
            if (pendingStarRemoval[id1]) {
                continue;
            }

            if (!entities.template HasComponent<CollisionComponent>(id1)) {
                continue;
            }

            CollisionComponent& collisionComponent1 = entities.template GetComponent<CollisionComponent>(id1);
            LocationComponent& locationComponent1 = entities.template GetComponent<LocationComponent>(id1);
            Rect worldBounds1 = collisionComponent1.collisionBox + locationComponent1.loc;

            for (auto it2 = std::next(it1); it2 != entities.end(); ++it2) {
                
                if (!it2->IsValid()) {
                    continue;
                }

                const EntityID id2 = it2->GetID();
                if (pendingStarRemoval[id1] || pendingStarRemoval[id2]) {
                    continue;
                }

                if (!entities.template HasComponent<CollisionComponent>(id2)) {
                    continue;
                }

                CollisionComponent& collisionComponent2 = entities.template GetComponent<CollisionComponent>(id2);
                LocationComponent& locationComponent2 = entities.template GetComponent<LocationComponent>(id2);
                Rect worldBounds2 = collisionComponent2.collisionBox + locationComponent2.loc;

                worldBounds2 &= worldBounds1;
                Rect intersect = worldBounds2;

                if (!std::isnan(intersect.width)) { // If bounding boxes intersect

                    if (collisionComponent1.staticEntity && !collisionComponent2.staticEntity) { // If 1 is static and 2 is dynamic

                        ResolveStaticDynamic(engine, entities, id2, id1, intersect,
                                             previousFlags[id2].onGround);

                    } else if (!collisionComponent1.staticEntity && collisionComponent2.staticEntity) { // If 1 is dynamic and 2 is static

                        ResolveStaticDynamic(engine, entities, id1, id2, intersect,
                                             previousFlags[id1].onGround);

                    } else if (!collisionComponent1.staticEntity && !collisionComponent2.staticEntity) { // If both are dynamic

                        const bool isPlayer1 = it1->GetName() == "player";
                        const bool isPlayer2 = it2->GetName() == "player";
                        const bool isStar1 = it1->GetName() == "star";
                        const bool isStar2 = it2->GetName() == "star";

                        if ((isPlayer1 && isStar2) || (isPlayer2 && isStar1)) {
                            // Collect after the loop finishes so removing the
                            // star cannot invalidate it1/it2.
                            const EntityID starID = isPlayer1 ? id2 : id1;
                            pendingStarRemoval[starID] = 1;

                            AddScore(entities, 10);
                            PlaySoundIfPresent(engine, "gem");
                        } else if (isStar1 && isStar2) {
                            // Separate two stars and swap their velocity along
                            // the axis of least overlap.
                            const float restitution = 0.85f;

                            if (intersect.width < intersect.height) {
                                const float halfOverlap = intersect.width * 0.5f;
                                if (locationComponent1.loc.x <= locationComponent2.loc.x) {
                                    locationComponent1.loc.x -= halfOverlap;
                                    locationComponent2.loc.x += halfOverlap;
                                } else {
                                    locationComponent1.loc.x += halfOverlap;
                                    locationComponent2.loc.x -= halfOverlap;
                                }

                                MovementComponent& movementComponent1 =
                                    entities.template GetComponent<MovementComponent>(id1);
                                MovementComponent& movementComponent2 =
                                    entities.template GetComponent<MovementComponent>(id2);

                                const float velocity1 = movementComponent1.velocity.x;
                                movementComponent1.velocity.x =
                                    movementComponent2.velocity.x * restitution;
                                movementComponent2.velocity.x = velocity1 * restitution;
                            } else {
                                const float halfOverlap = intersect.height * 0.5f;
                                if (locationComponent1.loc.y <= locationComponent2.loc.y) {
                                    locationComponent1.loc.y -= halfOverlap;
                                    locationComponent2.loc.y += halfOverlap;
                                } else {
                                    locationComponent1.loc.y += halfOverlap;
                                    locationComponent2.loc.y -= halfOverlap;
                                }

                                MovementComponent& movementComponent1 =
                                    entities.template GetComponent<MovementComponent>(id1);
                                MovementComponent& movementComponent2 =
                                    entities.template GetComponent<MovementComponent>(id2);

                                const float velocity1 = movementComponent1.velocity.y;
                                movementComponent1.velocity.y =
                                    movementComponent2.velocity.y * restitution;
                                movementComponent2.velocity.y = velocity1 * restitution;
                            }
                        }
                    }

                }

            }

        }

        for (EntityID id = 0; id < entityCount; ++id) {
            if (pendingStarRemoval[id]) {
                entities.RemoveEntity(id);
            }
        }

        return kContinue;
    }

private:
    // This is Travis's static/dynamic resolution with star bouncing and the
    // player landing sound added. Keeping it in one helper avoids duplicating
    // the same logic for the two static/dynamic orders.
    static void ResolveStaticDynamic(EngineView<Components...>* engine,
                                     EntityManager<Components...>& entities,
                                     EntityID dynamicID, EntityID staticID,
                                     const Rect& intersect, bool wasOnGround) {
        if (!entities.template HasComponent<MovementComponent>(dynamicID) ||
            !entities.template HasComponent<CollisionComponent>(dynamicID) ||
            !entities.template HasComponent<LocationComponent>(dynamicID) ||
            !entities.template HasComponent<LocationComponent>(staticID)) {
            return;
        }

        MovementComponent& movementComponent =
            entities.template GetComponent<MovementComponent>(dynamicID);
        CollisionComponent& collisionComponent =
            entities.template GetComponent<CollisionComponent>(dynamicID);
        LocationComponent& locationComponent =
            entities.template GetComponent<LocationComponent>(dynamicID);
        const LocationComponent& staticLocation =
            entities.template GetComponent<LocationComponent>(staticID);

        const bool isPlayer = entities.GetEntity(dynamicID)->GetName() == "player";
        const bool isStar = entities.GetEntity(dynamicID)->GetName() == "star";
        const float restitution = 0.85f;
        const float minBounceSpeed = 0.5f;

        if (intersect.width < intersect.height) { // If horizontal intersection is less than vertical

            const bool dynamicLeft = locationComponent.loc.x < staticLocation.loc.x;

            if (dynamicLeft) { // If dynamic entity is left of static entity
                locationComponent.loc.x -= intersect.width;
                collisionComponent.onRightWall = 1;
            } else { // If dynamic entity is right of static entity
                locationComponent.loc.x += intersect.width;
                collisionComponent.onLeftWall = 1;
            }

            if (isStar) {
                // Bounce off the wall only when moving into it.
                const bool movingIntoWall =
                    (dynamicLeft && movementComponent.velocity.x > 0.0f) ||
                    (!dynamicLeft && movementComponent.velocity.x < 0.0f);
                if (movingIntoWall) {
                    movementComponent.velocity.x =
                        -movementComponent.velocity.x * restitution;
                    if (std::fabs(movementComponent.velocity.x) < minBounceSpeed) {
                        movementComponent.velocity.x = 0.0f;
                    }
                }
            } else {
                movementComponent.velocity.x = 0;
            }

        } else { // If vertical intersection is less than or equal to horizontal

            const bool dynamicAbove = locationComponent.loc.y < staticLocation.loc.y;
            const float impactSpeed = movementComponent.velocity.y;

            if (dynamicAbove) { // If dynamic entity is above static entity
                locationComponent.loc.y -= intersect.height;
                collisionComponent.onGround = 1;

                if (isPlayer && !wasOnGround && impactSpeed > 0.5f) {
                    PlaySoundIfPresent(engine, "footstep-grass");
                }
            } else { // If dynamic entity is below static entity
                locationComponent.loc.y += intersect.height;
                collisionComponent.onCeiling = 1;
            }

            if (isStar) {
                // Bounce off the floor/ceiling only when moving into it.
                const bool movingIntoSurface =
                    (dynamicAbove && movementComponent.velocity.y > 0.0f) ||
                    (!dynamicAbove && movementComponent.velocity.y < 0.0f);
                if (movingIntoSurface) {
                    movementComponent.velocity.y =
                        -movementComponent.velocity.y * restitution;
                    if (std::fabs(movementComponent.velocity.y) < minBounceSpeed) {
                        movementComponent.velocity.y = 0.0f;
                    }
                }
            } else {
                movementComponent.velocity.y = 0;
            }
        }
    }

    // Adds points to the first score display that exists.
    static void AddScore(EntityManager<Components...>& entities, int points) {
        for (EntityID id = 0; id < entities.GetEntityCount(); ++id) {
            if (entities.IsValid(id) && entities.template HasComponent<ScoreComponent>(id)) {
                entities.template GetComponent<ScoreComponent>(id).score += points;
                return;
            }
        }
    }

    // Missing sound assets must not stop the engine tests.
    static void PlaySoundIfPresent(EngineView<Components...>* engine, const std::string& name) {
        SoundManager& sounds = engine->GetSoundManager();
        if (sounds.HasSound(name)) {
            sounds.PlaySound(name);
        }
    }
};

}  // namespace ECSEngine
