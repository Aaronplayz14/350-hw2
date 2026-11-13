#pragma once

#include "ECSEngine.h"

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Saves each collision box in world coordinates before movement runs.
 *
 * Collision resolution needs to know where an entity was before this frame's
 * movement so that collision resolution can identify a crossed face, falling
 * back to the least penetration for existing overlaps. This pass stores the
 * location-adjusted box in CollisionComponent::previousCollisionBox.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class CollisionLateSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionLateSystem");
        return name;
    }

    //-------------------------------------------------------------------
    /**
     * @brief Snapshots valid collision/location pairs before input and movement.
     * @param engine Borrowed view; entity/component references stay local to this call.
     * @return kContinue after copying the world-space bounds.
     */
    //-------------------------------------------------------------------
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
