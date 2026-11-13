#pragma once

namespace ECSEngine
{

template <typename... Components>
class GravitySystem : public System<Components...> {
public:
	const std::string & GetName() const override
	{ static std::string name("GravitySystem"); return name; }
	
	tSystemReturn Run(EngineView<Components...> *engine) override
	{

		EntityManager<Components...>& entities = engine->GetEntityManager();

        for (auto it = entities.begin(); it != entities.end(); ++it) {

            if (!it->IsValid()) {
                continue;
            }

            const EntityID id = it->GetID();
            if (!entities.template HasComponent<GravityComponent>(id)) {
                continue;
            }

            GravityComponent& gravityComponent = entities.template GetComponent<GravityComponent>(id);
			MovementComponent& movementComponent = entities.template GetComponent<MovementComponent>(id);

			movementComponent.velocity += gravityComponent.acceleration;
        }

		return kContinue;
	}
	
};


}



