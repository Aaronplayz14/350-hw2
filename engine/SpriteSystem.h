#pragma once

#include <string>

#include "ECSEngine.h"

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Draws every live entity that has a SpriteComponent.
 *
 * World-space sprites are offset by the entity's LocationComponent and then
 * converted to window coordinates through WindowManager. Screen-space sprites
 * (the score display, for example) are drawn directly in window coordinates.
 * The source rectangle comes from SpriteManager, while
 * SpriteComponent::bounds controls where and at what size the image is drawn.
 *
 * This system clears the window, draws all sprites, and finally calls
 * display(), so it is expected to be the last drawing system in the frame.
 * Components whose texture id is invalid are skipped rather than throwing, so
 * a partially constructed scene still renders.
 *
 * Implemented with the help of AI.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class SpriteSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpriteSystem");
        return name;
    }

    //-------------------------------------------------------------------
    /**
     * @brief Draws one frame of sprites and presents the window.
     *
     * Bounds are relative to the entity location. A zero width or height in
     * SpriteComponent::bounds means "use the texture's natural pixel size";
     * otherwise the sprite is scaled to the requested box.
     *
     * Implemented with the help of AI.
     *
     * @param engine Engine view providing the window, entities, and sprites.
     * @return kContinue; drawing never requests an engine exit.
     */
    //-------------------------------------------------------------------
    tSystemReturn Run(EngineView<Components...>* engine) override {
        WindowManager& windowManager = engine->GetWindowManager();
        auto window = windowManager.GetWindow();
        if (!window) {
            return kContinue;
        }

        EntityManager<Components...>& entities = engine->GetEntityManager();
        SpriteManager& sprites = engine->GetSpriteManager();

        window->clear();

        for (auto it = entities.begin(); it != entities.end(); ++it) {
            if (!it->IsValid()) {
                continue;
            }

            const EntityID id = it->GetID();
            if (!entities.template HasComponent<SpriteComponent>(id)) {
                continue;
            }

            const SpriteComponent& spriteComponent =
                entities.template GetComponent<SpriteComponent>(id);
            if (!sprites.IsValid(spriteComponent.textureID)) {
                continue;
            }

            sf::Sprite& sprite = sprites.GetSprite(spriteComponent.textureID);
            const sf::IntRect& textureRect = sprite.getTextureRect();
            if (textureRect.size.x <= 0 || textureRect.size.y <= 0) {
                continue;
            }

            // The component bounds are relative to the entity location.
            Rect drawBounds = spriteComponent.bounds;
            if (entities.template HasComponent<LocationComponent>(id)) {
                const Point2D& location =
                    entities.template GetComponent<LocationComponent>(id).loc;
                drawBounds.topLeft.x += location.x;
                drawBounds.topLeft.y += location.y;
            }

            if (spriteComponent.worldSpace) {
                drawBounds = windowManager.WorldToWindow(drawBounds);
            }

            // A zero-sized component rect means "draw at the texture's natural
            // pixel size".
            const float width = drawBounds.width > 0.0F
                                    ? drawBounds.width
                                    : static_cast<float>(textureRect.size.x);
            const float height = drawBounds.height > 0.0F
                                     ? drawBounds.height
                                     : static_cast<float>(textureRect.size.y);

            sprite.setPosition({drawBounds.topLeft.x, drawBounds.topLeft.y});
            sprite.setScale(
                {width / static_cast<float>(textureRect.size.x),
                 height / static_cast<float>(textureRect.size.y)});
            window->draw(sprite);
        }

        window->display();
        return kContinue;
    }
};

}  // namespace ECSEngine
