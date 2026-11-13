#pragma once

#include <unordered_map>

#include "LocationComponent.h"
#include "MathUtil.h"
#include "SpriteComponent.h"
#include "SystemManager.h"
#include "ECSEngine.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

template <typename... Components>
class CameraSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name = "CameraSystem";
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        
        WindowManager& windowManager = engine->GetWindowManager();
        EntityManager<Components...>& entities = engine->GetEntityManager();

        for (auto it1 = entities.begin(); it1 != entities.end(); ++it1) {

            if (!it1->IsValid()) {
                continue;
            }

            const EntityID idCam = it1->GetID();
            if (!entities.template HasComponent<CameraComponent>(idCam)) {
                continue;
            }

            CameraComponent& cameraComponent = entities.template GetComponent<CameraComponent>(idCam);

            

            for (auto it2 = entities.begin(); it2 != entities.end(); ++it2) {

                if (!it2->IsValid()) {
                    continue;
                }

                const EntityID idFol = it2->GetID();
                if (!entities.template HasComponent<CameraFollower>(idFol)) {
                    continue;
                }

                const LocationComponent& locationComponent = entities.template GetComponent<LocationComponent>(idFol);

                cameraComponent.position.x = locationComponent.loc.x;

                windowManager.SetWorldCenter(cameraComponent.position);
            }
        }
        return kContinue; 
    }
};

}  // namespace ECSEngine
