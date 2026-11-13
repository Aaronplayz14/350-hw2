#pragma once

#include <algorithm>

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

			// Falling should be faster than rising. This keeps Travis's
			// acceleration value and only strengthens it on the way down.
			Point2D acceleration = gravityComponent.acceleration;
			if (acceleration.y > 0.0f && movementComponent.velocity.y > 0.0f) {
				acceleration.y *= 1.6f;
			}

			movementComponent.velocity += acceleration;
            if (it->GetName() == "star") {
                // Keep a small gravity step even at rest so CollisionSystem
                // verifies floor support each frame; stars fall off ledges.
                movementComponent.velocity.y = std::min(movementComponent.velocity.y, 12.0F);
            }
        }

		return kContinue;
	}
	
};


}



