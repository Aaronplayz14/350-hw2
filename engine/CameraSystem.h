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

//-----------------------------------------------------------------------
/**
 * @brief Positions the camera to follow entities with a CameraFollower.
 *
 * CameraSystem currently copies the follower's x coordinate into the camera
 * position and leaves y alone. The project requirements call for horizontal
 * follow bands and a fixed y; those details are marked with temporary notes
 * below while that part of the camera work is still in progress.
 */
//-----------------------------------------------------------------------
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

            // TEMP (Travis): This is the spot for the remaining camera work.
            // Current behavior is intentionally minimal: the camera snaps its
            // x coordinate to the follower and leaves y at the level center.
            //
            // Intended behavior from the project requirements:
            // 1. keep y fixed and follow only on the x axis;
            // 2. keep the player out of the outer 10% of the screen width;
            // 3. in the 10-30% side bands, tween the camera slowly;
            // 4. when the player stops, keep easing until they are inside the
            //    center 40% of the screen.
            //
            // These notes can be deleted after the camera implementation lands.

            for (auto it2 = entities.begin(); it2 != entities.end(); ++it2) {

                if (!it2->IsValid()) {
                    continue;
                }

                const EntityID idFol = it2->GetID();
                if (!entities.template HasComponent<CameraFollower>(idFol)) {
                    continue;
                }

                const LocationComponent& locationComponent = entities.template GetComponent<LocationComponent>(idFol);

                Rect worldHard = windowManager.WindowToWorld(cameraComponent.hardExclusion);
                Rect worldSoft = windowManager.WindowToWorld(cameraComponent.softExclusion);

                if (locationComponent.loc.x < worldHard.topLeft.x) {
                    cameraComponent.position.x = locationComponent.loc.x + worldHard.width / 2;
                } else if (locationComponent.loc.x > worldHard.topLeft.x + worldHard.width) {
                    cameraComponent.position.x = locationComponent.loc.x - worldHard.width / 2;
                } else if (locationComponent.loc.x < worldSoft.topLeft.x) {
                    float distFromZone = cameraComponent.position.x - locationComponent.loc.x - worldSoft.width / 2;
                    cameraComponent.position.x -= distFromZone * cameraComponent.speed;
                } else if (locationComponent.loc.x > worldSoft.topLeft.x + worldSoft.width) {
                    float distFromZone = locationComponent.loc.x - cameraComponent.position.x - worldSoft.width / 2;
                    cameraComponent.position.x += distFromZone * cameraComponent.speed;
                }

                windowManager.SetWorldCenter(cameraComponent.position);
            }
        }
        return kContinue; 
    }
};

}  // namespace ECSEngine
