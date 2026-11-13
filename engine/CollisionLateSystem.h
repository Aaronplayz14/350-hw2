#pragma once

namespace ECSEngine {

template <typename... Components>
class CollisionLateSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionLateSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override { 

        for (auto it = entities.begin(); it != entities.end(); ++it) {

            if (!it->IsValid()) {
                continue;
            }

            const EntityID id = it->GetID();
            if (!entities.template HasComponent<CollisionComponent>(id)) {
                continue;
            }

            CollisionComponent& collisionComponent = entities.template GetComponent<CollisionComponent>(id);
            LocationComponent& locationComponent = entities.template GetComponent<LocationComponent>(id);

            collisionComponent.previousCollisionBox = collisionComponent.collisionBox + locationComponent.loc;
        }

        return kContinue; 
    }


};

}  // namespace ECSEngine
