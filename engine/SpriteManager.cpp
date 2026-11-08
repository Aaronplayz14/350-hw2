#include "SpriteManager.h"

namespace ECSEngine {

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    return 0;
}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    static sf::Texture dummyTexture;
    static sf::Sprite dummy(dummyTexture);
    return dummy;
}

}  // namespace ECSEngine
