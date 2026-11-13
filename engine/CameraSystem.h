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
 * Uses Travis's hard/soft horizontal follow bands with a fixed base y.
 * Collision shake is a temporary display offset; it never changes the base
 * camera position or the follow-band calculations.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class CameraSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name = "CameraSystem";
        return name;
    }

    //-------------------------------------------------------------------
    /**
     * @brief Follows target locations horizontally and applies one frame of shake.
     * @param engine Borrowed managers; CameraFollower::entityID identifies the target.
     * @return kContinue after setting the window's display center.
     */
    //-------------------------------------------------------------------
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

            // Restore the unshaken center before converting screen-space
            // follow bands. Last frame's shake must not steer the camera.
            windowManager.SetWorldScale(cameraComponent.worldUnitsPerPixel);
            windowManager.SetWorldCenter(cameraComponent.position);

            for (auto it2 = entities.begin(); it2 != entities.end(); ++it2) {

                if (!it2->IsValid()) {
                    continue;
                }

                const EntityID follower = it2->GetID();
                if (!entities.template HasComponent<CameraFollower>(follower)) {
                    continue;
                }

                const EntityID idFol = entities.template GetComponent<CameraFollower>(follower).entityID;
                if (!entities.IsValid(idFol) ||
                    !entities.template HasComponent<LocationComponent>(idFol)) continue;

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

            }

            // Sample and advance once per camera frame, even without a
            // follower, and return exactly to the base position on expiry.
            Point2D displayedPosition = cameraComponent.position;
            if (entities.template HasComponent<CameraShake>(idCam)) {
                CameraShake& shake = entities.template GetComponent<CameraShake>(idCam);
                displayedPosition += shake.GetOffset();
                if (--shake.frameLimit <= 0) {
                    entities.template RemoveComponent<CameraShake>(idCam);
                }
            }
            windowManager.SetWorldCenter(displayedPosition);
        }
        return kContinue; 
    }
};

}  // namespace ECSEngine
