#pragma once

#include "ECSEngine.h"

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Saves each collision box in world coordinates before movement runs.
 *
 * Collision resolution needs to know where an entity was before this frame's
 * movement so that overlap can be resolved along the axis of least
 * penetration. This pass runs first in the system order and stores the
 * location-adjusted box in CollisionComponent::previousCollisionBox.
 *
 * Implemented with the help of AI.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class CollisionLateSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionLateSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        EntityManager<Components...>& entities = engine->GetEntityManager();

        for (auto it = entities.begin(); it != entities.end(); ++it) {
            if (!it->IsValid()) {
                continue;
            }

            const EntityID id = it->GetID();
            if (!entities.template HasComponent<CollisionComponent>(id) ||
                !entities.template HasComponent<LocationComponent>(id)) {
                continue;
            }

            CollisionComponent& collisionComponent =
                entities.template GetComponent<CollisionComponent>(id);
            const LocationComponent& locationComponent =
                entities.template GetComponent<LocationComponent>(id);

            collisionComponent.previousCollisionBox =
                collisionComponent.collisionBox + locationComponent.loc;
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
