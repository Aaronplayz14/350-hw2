#pragma once

#include <iterator>

namespace ECSEngine {

template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        
        EntityManager<Components...>& entities = engine->GetEntityManager();

        for (auto it1 = entities.begin(); std::next(it1) != entities.end(); ++it1) {

            if (!it1->IsValid()) {
                continue;
            }

            const EntityID id1 = it1->GetID();
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

                        MovementComponent& movementComponent = entities.template GetComponent<MovementComponent>(id2);

                        if (intersect.width < intersect.height) { // If horizontal intersection is less than vertical

                            movementComponent.velocity.x = 0;

                            if (locationComponent2.loc.x < locationComponent1.loc.x) { // If dynamic entity is left of static entity
                                locationComponent2.loc.x -= intersect.width;
                                collisionComponent2.onRightWall = 1;
                            } else { // If dynamic entity is right of static entity
                                locationComponent2.loc.x += intersect.width;
                                collisionComponent2.onLeftWall = 1;
                            }

                        } else { // If vertical intersection is less than or equal to horizontal

                            movementComponent.velocity.y = 0;

                            if (locationComponent2.loc.y < locationComponent1.loc.y) { // If dynamic entity is above static entity
                                locationComponent2.loc.y -= intersect.height;
                                collisionComponent2.onGround = 1;
                            } else { // If dynamic entity is below static entity
                                locationComponent2.loc.y += intersect.height;
                                collisionComponent2.onCeiling = 1;
                            }
                        }

                    } else if (!collisionComponent1.staticEntity && collisionComponent2.staticEntity) { // If 1 is dynamic and 2 is static

                        MovementComponent& movementComponent = entities.template GetComponent<MovementComponent>(id1);

                        if (intersect.width < intersect.height) { // If horizontal intersection is less than vertical

                            movementComponent.velocity.x = 0;

                            if (locationComponent1.loc.x < locationComponent2.loc.x) { // If dynamic entity is left of static entity
                                locationComponent1.loc.x -= intersect.width;
                                collisionComponent1.onRightWall = 1;
                            } else { // If dynamic entity is right of static entity
                                locationComponent1.loc.x += intersect.width;
                                collisionComponent1.onLeftWall = 1;
                            }

                        } else { // If vertical intersection is less than or equal to horizontal

                            movementComponent.velocity.y = 0;

                            if (locationComponent1.loc.y < locationComponent2.loc.y) { // If dynamic entity is above static entity
                                locationComponent1.loc.y -= intersect.height;
                                collisionComponent1.onGround = 1;
                            } else { // If dynamic entity is below static entity
                                locationComponent1.loc.y += intersect.height;
                                collisionComponent1.onCeiling = 1;
                            }
                        }

                    } else if (!collisionComponent1.staticEntity && !collisionComponent2.staticEntity) { // If both are dynamic
                        // TODO: Implement player/star collision handling
                    }

                }
                
            }

        }

        return kContinue; 
    }
};

}  // namespace ECSEngine
