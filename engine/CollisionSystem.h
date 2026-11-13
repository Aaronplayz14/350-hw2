#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "ECSEngine.h"

namespace ECSEngine {

/**
 * @brief Resolves static/dynamic and star/player collisions.
 *
 * The system uses bounds saved before movement to identify the face crossed,
 * rather than relying only on the smallest overlap after movement. Players
 * stop at solid tiles, respawn at damaging tiles, and collect stars. Stars
 * bounce from static tiles and other stars while losing speed.
 */
template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static const std::string name("CollisionSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        EntityManager<Components...>& entities = engine->GetEntityManager();
        const std::size_t entityCount = entities.GetEntityCount();

        // InputSystem consumes the preceding frame's flags. Clear them only
        // after it has run, while preserving ground state for landing audio.
        std::vector<PreviousFlags> previousFlags(entityCount);
        std::vector<std::uint8_t> pendingStarRemoval(entityCount, 0);
        for (EntityID id = 0; id < entityCount; ++id) {
            if (!IsCollidable(entities, id)) {
                continue;
            }
            CollisionComponent& collision =
                entities.template GetComponent<CollisionComponent>(id);
            previousFlags[id] = PreviousFlags{collision.onGround, collision.onCeiling,
                                              collision.onLeftWall, collision.onRightWall};
            collision.onGround = 0;
            collision.onCeiling = 0;
            collision.onLeftWall = 0;
            collision.onRightWall = 0;
            collision.resting = 0;
        }

        for (EntityID first = 0; first < entityCount; ++first) {
            if (!IsCollidable(entities, first) || pendingStarRemoval[first]) {
                continue;
            }

            for (EntityID second = first + 1; second < entityCount; ++second) {
                if (!IsCollidable(entities, second) || pendingStarRemoval[second]) {
                    continue;
                }

                // Rebuild bounds for each pair because an earlier resolution
                // may already have moved either dynamic entity.
                Rect overlap = WorldBounds(entities, first);
                overlap &= WorldBounds(entities, second);
                if (std::isnan(overlap.width)) {
                    continue;
                }

                CollisionComponent& firstCollision =
                    entities.template GetComponent<CollisionComponent>(first);
                CollisionComponent& secondCollision =
                    entities.template GetComponent<CollisionComponent>(second);

                if (firstCollision.staticEntity != secondCollision.staticEntity) {
                    const EntityID dynamicID = firstCollision.staticEntity ? second : first;
                    const EntityID staticID = firstCollision.staticEntity ? first : second;

                    // Spawner tiles are solid level geometry for the player,
                    // but stars must fall through their source tile instead
                    // of bouncing and becoming parked on top of it.
                    const Entity* dynamicEntity = entities.GetEntity(dynamicID);
                    if (dynamicEntity != nullptr && dynamicEntity->GetName() == "star" &&
                        entities.template HasComponent<SpawnComponent>(staticID)) {
                        continue;
                    }
                    ResolveStaticDynamic(engine, entities, dynamicID, staticID, overlap,
                                         previousFlags[dynamicID]);
                } else if (!firstCollision.staticEntity) {
                    ResolveDynamicPair(engine, entities, first, second, overlap,
                                       pendingStarRemoval);
                }
            }
        }

        // Defer removals until all entity ids have been examined.
        for (EntityID id = 0; id < entityCount; ++id) {
            if (pendingStarRemoval[id] && entities.IsValid(id)) {
                entities.RemoveEntity(id);
            }
        }
        return kContinue;
    }

private:
    struct PreviousFlags {
        std::uint8_t onGround = 0;
        std::uint8_t onCeiling = 0;
        std::uint8_t onLeftWall = 0;
        std::uint8_t onRightWall = 0;
    };

    // Stars lose much more energy than the player on every hit. Ground hits
    // are damped hardest so a star settles instead of bouncing around forever.
    static constexpr float kStarWallBounce = 0.5F;
    static constexpr float kStarGroundBounce = 0.28F;
    static constexpr float kStarCeilingBounce = 0.4F;
    static constexpr float kStarGroundFriction = 0.78F;
    static constexpr float kStarStarDamping = 0.7F;
    static constexpr float kStarRestVerticalSpeed = 1.2F;
    static constexpr float kStarRestHorizontalSpeed = 1.0F;
    static constexpr float kMinStarSpeed = 0.1F;
    static constexpr float kSideEpsilon = 0.01F;
    static constexpr int kShakeFrames = 14;
    static constexpr float kMaxShakeMagnitude = 4.0F;

    enum class CollisionSide { Left, Right, Top, Bottom };

    static bool IsCollidable(const EntityManager<Components...>& entities, EntityID id) {
        return entities.IsValid(id) &&
               entities.template HasComponent<CollisionComponent>(id) &&
               entities.template HasComponent<LocationComponent>(id);
    }

    static Rect WorldBounds(const EntityManager<Components...>& entities, EntityID id) {
        const CollisionComponent& collision =
            entities.template GetComponent<CollisionComponent>(id);
        const LocationComponent& location =
            entities.template GetComponent<LocationComponent>(id);
        return collision.collisionBox + location.loc;
    }

    static CollisionSide DetermineSide(const CollisionComponent& dynamicCollision,
                                       const Rect& dynamicBounds, const Rect& staticBounds,
                                       const Rect& overlap) {
        const Rect& previousBounds = dynamicCollision.previousCollisionBox;
        const float deltaX = dynamicBounds.topLeft.x - previousBounds.topLeft.x;
        const float deltaY = dynamicBounds.topLeft.y - previousBounds.topLeft.y;
        float firstContact = 2.0F;
        CollisionSide enteredSide = CollisionSide::Top;

        // At a diagonal corner, the face whose boundary the entity crossed
        // first is the correct one. This is why saved pre-movement bounds are
        // useful: the current overlap alone cannot distinguish the two faces.
        const auto consider = [&firstContact, &enteredSide](float time, CollisionSide side) {
            if (time >= -kSideEpsilon && time <= 1.0F + kSideEpsilon && time < firstContact) {
                firstContact = time;
                enteredSide = side;
            }
        };
        if (deltaX > kSideEpsilon &&
            previousBounds.topLeft.x + previousBounds.width <=
                staticBounds.topLeft.x + kSideEpsilon) {
            consider((staticBounds.topLeft.x -
                      (previousBounds.topLeft.x + previousBounds.width)) / deltaX,
                     CollisionSide::Left);
        }
        if (deltaX < -kSideEpsilon &&
            previousBounds.topLeft.x >=
                staticBounds.topLeft.x + staticBounds.width - kSideEpsilon) {
            consider((staticBounds.topLeft.x + staticBounds.width - previousBounds.topLeft.x) /
                         deltaX,
                     CollisionSide::Right);
        }
        if (deltaY > kSideEpsilon &&
            previousBounds.topLeft.y + previousBounds.height <=
                staticBounds.topLeft.y + kSideEpsilon) {
            consider((staticBounds.topLeft.y -
                      (previousBounds.topLeft.y + previousBounds.height)) / deltaY,
                     CollisionSide::Top);
        }
        if (deltaY < -kSideEpsilon &&
            previousBounds.topLeft.y >=
                staticBounds.topLeft.y + staticBounds.height - kSideEpsilon) {
            consider((staticBounds.topLeft.y + staticBounds.height - previousBounds.topLeft.y) /
                         deltaY,
                     CollisionSide::Bottom);
        }
        if (firstContact <= 1.0F + kSideEpsilon) {
            return enteredSide;
        }

        // Teleported/spawned entities can start inside a tile. Use the least
        // penetration and the current centre as a deterministic fallback.
        if (overlap.width < overlap.height) {
            const float dynamicCenter = dynamicBounds.topLeft.x + dynamicBounds.width * 0.5F;
            const float staticCenter = staticBounds.topLeft.x + staticBounds.width * 0.5F;
            return dynamicCenter < staticCenter ? CollisionSide::Left : CollisionSide::Right;
        }
        const float dynamicCenter = dynamicBounds.topLeft.y + dynamicBounds.height * 0.5F;
        const float staticCenter = staticBounds.topLeft.y + staticBounds.height * 0.5F;
        return dynamicCenter < staticCenter ? CollisionSide::Top : CollisionSide::Bottom;
    }

    static void ResolveStaticDynamic(EngineView<Components...>* engine,
                                     EntityManager<Components...>& entities,
                                     EntityID dynamicID, EntityID staticID,
                                     const Rect& overlap,
                                     const PreviousFlags& previousFlags) {
        if (!entities.template HasComponent<MovementComponent>(dynamicID)) {
            return;
        }

        CollisionComponent& dynamicCollision =
            entities.template GetComponent<CollisionComponent>(dynamicID);
        const CollisionComponent& staticCollision =
            entities.template GetComponent<CollisionComponent>(staticID);
        LocationComponent& dynamicLocation =
            entities.template GetComponent<LocationComponent>(dynamicID);
        MovementComponent& dynamicMovement =
            entities.template GetComponent<MovementComponent>(dynamicID);
        const CollisionSide side = DetermineSide(dynamicCollision, WorldBounds(entities, dynamicID),
                                                 WorldBounds(entities, staticID), overlap);

        const Entity* dynamicEntity = entities.GetEntity(dynamicID);
        const bool isPlayer = dynamicEntity != nullptr && dynamicEntity->GetName() == "player";
        const bool isStar = dynamicEntity != nullptr && dynamicEntity->GetName() == "star";

        if (isPlayer && staticCollision.doesDamage) {
            RespawnPlayer(entities, dynamicID);
            PlaySoundIfPresent(engine, "error");
            return;
        }

        const float impactSpeed = side == CollisionSide::Left || side == CollisionSide::Right
                                      ? std::fabs(dynamicMovement.velocity.x)
                                      : std::fabs(dynamicMovement.velocity.y);
        switch (side) {
            case CollisionSide::Left:
                dynamicLocation.loc.x -= overlap.width;
                dynamicCollision.onRightWall = 1;
                ResolveVelocity(dynamicMovement, isStar, true, true);
                break;
            case CollisionSide::Right:
                dynamicLocation.loc.x += overlap.width;
                dynamicCollision.onLeftWall = 1;
                ResolveVelocity(dynamicMovement, isStar, true, false);
                break;
            case CollisionSide::Top:
                dynamicLocation.loc.y -= overlap.height;
                dynamicCollision.onGround = 1;
                if (isPlayer && !previousFlags.onGround && dynamicMovement.velocity.y > 0.5F) {
                    PlaySoundIfPresent(engine, "footstep-grass");
                }
                ResolveVelocity(dynamicMovement, isStar, false, true);
                break;
            case CollisionSide::Bottom:
                dynamicLocation.loc.y += overlap.height;
                dynamicCollision.onCeiling = 1;
                ResolveVelocity(dynamicMovement, isStar, false, false);
                break;
        }

        if (isStar) {
            dynamicCollision.resting = dynamicCollision.onGround &&
                                      dynamicMovement.velocity.x == 0.0F &&
                                      dynamicMovement.velocity.y == 0.0F;
        }
        if (isPlayer && impactSpeed > 3.0F && !HadContactOnSide(previousFlags, side)) {
            TriggerScreenShake(entities, side, impactSpeed);
        }
    }

    static bool HadContactOnSide(const PreviousFlags& flags, CollisionSide side) {
        switch (side) {
            case CollisionSide::Left: return flags.onRightWall != 0;
            case CollisionSide::Right: return flags.onLeftWall != 0;
            case CollisionSide::Top: return flags.onGround != 0;
            case CollisionSide::Bottom: return flags.onCeiling != 0;
        }
        return false;
    }

    static void ResolveVelocity(MovementComponent& movement, bool isStar, bool horizontal,
                                bool hitPositiveFace) {
        float& normalVelocity = horizontal ? movement.velocity.x : movement.velocity.y;
        const bool movingIntoSurface = hitPositiveFace ? normalVelocity > 0.0F
                                                       : normalVelocity < 0.0F;
        if (isStar) {
            if (movingIntoSurface) {
                const float restitution = horizontal ? kStarWallBounce
                    : (hitPositiveFace ? kStarGroundBounce : kStarCeilingBounce);
                normalVelocity = -normalVelocity * restitution;
                if (!horizontal && hitPositiveFace) {
                    movement.velocity.x *= kStarGroundFriction;
                    if (std::fabs(normalVelocity) < kStarRestVerticalSpeed) {
                        normalVelocity = 0.0F;
                    }
                    if (std::fabs(movement.velocity.x) < kStarRestHorizontalSpeed) {
                        movement.velocity.x = 0.0F;
                    }
                }
            }
        } else {
            normalVelocity = 0.0F;
        }
    }

    static void ResolveDynamicPair(EngineView<Components...>* engine,
                                   EntityManager<Components...>& entities, EntityID first,
                                   EntityID second, const Rect& overlap,
                                   std::vector<std::uint8_t>& pendingStarRemoval) {
        const Entity* firstEntity = entities.GetEntity(first);
        const Entity* secondEntity = entities.GetEntity(second);
        const bool firstPlayer = firstEntity != nullptr && firstEntity->GetName() == "player";
        const bool secondPlayer = secondEntity != nullptr && secondEntity->GetName() == "player";
        const bool firstStar = firstEntity != nullptr && firstEntity->GetName() == "star";
        const bool secondStar = secondEntity != nullptr && secondEntity->GetName() == "star";

        if ((firstPlayer && secondStar) || (secondPlayer && firstStar)) {
            pendingStarRemoval[firstPlayer ? second : first] = 1;
            AddScore(entities, 10);
            PlaySoundIfPresent(engine, "gem");
            return;
        }

        if (!firstStar || !secondStar ||
            !entities.template HasComponent<MovementComponent>(first) ||
            !entities.template HasComponent<MovementComponent>(second)) {
            return;
        }

        LocationComponent& firstLocation =
            entities.template GetComponent<LocationComponent>(first);
        LocationComponent& secondLocation =
            entities.template GetComponent<LocationComponent>(second);
        MovementComponent& firstMovement =
            entities.template GetComponent<MovementComponent>(first);
        MovementComponent& secondMovement =
            entities.template GetComponent<MovementComponent>(second);

        bool approaching = false;
        if (overlap.width < overlap.height) {
            const float displacement = overlap.width * 0.5F;
            const bool firstOnLeft = firstLocation.loc.x <= secondLocation.loc.x;
            firstLocation.loc.x += firstOnLeft ? -displacement : displacement;
            secondLocation.loc.x += firstOnLeft ? displacement : -displacement;
            approaching = firstOnLeft ? firstMovement.velocity.x > secondMovement.velocity.x
                                      : firstMovement.velocity.x < secondMovement.velocity.x;
            if (approaching) std::swap(firstMovement.velocity.x, secondMovement.velocity.x);
        } else {
            const float displacement = overlap.height * 0.5F;
            const bool firstAbove = firstLocation.loc.y <= secondLocation.loc.y;
            firstLocation.loc.y += firstAbove ? -displacement : displacement;
            secondLocation.loc.y += firstAbove ? displacement : -displacement;
            approaching = firstAbove ? firstMovement.velocity.y > secondMovement.velocity.y
                                     : firstMovement.velocity.y < secondMovement.velocity.y;
            if (approaching) std::swap(firstMovement.velocity.y, secondMovement.velocity.y);
        }
        if (approaching) {
            DampStarVelocity(firstMovement);
            DampStarVelocity(secondMovement);
            entities.template GetComponent<CollisionComponent>(first).resting = 0;
            entities.template GetComponent<CollisionComponent>(second).resting = 0;
        }
    }

    static void RespawnPlayer(EntityManager<Components...>& entities, EntityID id) {
        if (!entities.template HasComponent<RespawnComponent>(id) ||
            !entities.template HasComponent<MovementComponent>(id)) {
            return;
        }
        entities.template GetComponent<LocationComponent>(id).loc =
            entities.template GetComponent<RespawnComponent>(id).loc;
        entities.template GetComponent<MovementComponent>(id).velocity = Point2D(0.0F, 0.0F);
        CollisionComponent& collision = entities.template GetComponent<CollisionComponent>(id);
        collision.onGround = 0;
        collision.onCeiling = 0;
        collision.onLeftWall = 0;
        collision.onRightWall = 0;
    }

    static void TriggerScreenShake(EntityManager<Components...>& entities, CollisionSide side,
                                   float impactSpeed) {
        const float magnitude = std::min(kMaxShakeMagnitude, impactSpeed * 0.18F);
        Point2D offset;
        switch (side) {
            case CollisionSide::Left: offset.x = magnitude; break;
            case CollisionSide::Right: offset.x = -magnitude; break;
            case CollisionSide::Top: offset.y = magnitude; break;
            case CollisionSide::Bottom: offset.y = -magnitude; break;
        }

        for (EntityID id = 0; id < entities.GetEntityCount(); ++id) {
            if (entities.IsValid(id) && entities.template HasComponent<CameraComponent>(id)) {
                if (entities.template HasComponent<CameraShake>(id)) {
                    const CameraShake& current = entities.template GetComponent<CameraShake>(id);
                    const Point2D currentOffset = current.GetOffset();
                    if (currentOffset.Dot(currentOffset) > offset.Dot(offset)) return;
                }
                entities.template AddComponent<CameraShake>(id,
                    CameraShake{kShakeFrames, offset, kShakeFrames});
                return;
            }
        }
    }

    static void DampStarVelocity(MovementComponent& movement) {
        movement.velocity.x *= kStarStarDamping;
        movement.velocity.y *= kStarStarDamping;
        if (std::fabs(movement.velocity.x) < kMinStarSpeed) movement.velocity.x = 0.0F;
        if (std::fabs(movement.velocity.y) < kMinStarSpeed) movement.velocity.y = 0.0F;
    }

    static void AddScore(EntityManager<Components...>& entities, int points) {
        for (EntityID id = 0; id < entities.GetEntityCount(); ++id) {
            if (entities.IsValid(id) && entities.template HasComponent<ScoreComponent>(id)) {
                entities.template GetComponent<ScoreComponent>(id).score += points;
                return;
            }
        }
    }

    static void PlaySoundIfPresent(EngineView<Components...>* engine, const std::string& name) {
        SoundManager& sounds = engine->GetSoundManager();
        if (sounds.HasSound(name)) sounds.PlaySound(name);
    }
};

}  // namespace ECSEngine
