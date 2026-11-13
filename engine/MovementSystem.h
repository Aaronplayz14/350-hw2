#pragma once

namespace ECSEngine {

template <typename... Components>
class MovementSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("MovementSystem");
        return name;
    }

    //-------------------------------------------------------------------
    /**
     * @brief Adds each entity's velocity to its location once per simulation frame.
     * @param engine Borrowed view; moving entities must also have LocationComponent.
     * @return kContinue; collision flags are left for CollisionSystem to update.
     */
    //-------------------------------------------------------------------
    tSystemReturn Run(EngineView<Components...>* engine) override {
        
        EntityManager<Components...>& entities = engine->GetEntityManager();

        for (auto it = entities.begin(); it != entities.end(); ++it) {

            if (!it->IsValid()) {
                continue;
            }

            const EntityID id = it->GetID();
            if (!entities.template HasComponent<MovementComponent>(id)) {
                continue;
            }

            MovementComponent& movementComponent = entities.template GetComponent<MovementComponent>(id);
            LocationComponent& locationComponent = entities.template GetComponent<LocationComponent>(id);

            locationComponent.loc += movementComponent.velocity;

            // Collision flags are now cleared by CollisionSystem instead of
            // here. InputSystem needs last frame's flags before CollisionSystem
            // runs, and CollisionSystem needs them to detect a fresh landing.
        }

        return kContinue; 
    }
};

}  // namespace ECSEngine
