#include "SpriteManager.h"

#include <stdexcept>
#include <utility>

namespace ECSEngine {

namespace {

// Converts the engine's float Rect into the integer rectangle SFML expects.
sf::IntRect ToIntRect(const Rect& rect) {
    return sf::IntRect({static_cast<int>(rect.topLeft.x), static_cast<int>(rect.topLeft.y)},
                       {static_cast<int>(rect.width), static_cast<int>(rect.height)});
}

}  // namespace

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    if (texturePath.empty()) {
        throw std::invalid_argument("SpriteManager::RegisterTexture requires a non-empty path");
    }

    auto found = mTextures.find(texturePath);
    sf::Texture* texture = nullptr;

    if (found != mTextures.end()) {
        texture = found->second.get();
    } else {
        auto newTexture = std::make_unique<sf::Texture>();
        if (!newTexture->loadFromFile(texturePath)) {
            throw std::runtime_error("SpriteManager could not load texture: " + texturePath);
        }
        texture = newTexture.get();
        mTextures.emplace(texturePath, std::move(newTexture));
    }

    sf::IntRect textureRect = ToIntRect(sourceRect);

    // An empty/unspecified source rectangle means the whole texture.
    if (textureRect.size.x <= 0 || textureRect.size.y <= 0) {
        const sf::Vector2u size = texture->getSize();
        textureRect = sf::IntRect({0, 0}, {static_cast<int>(size.x), static_cast<int>(size.y)});
    }

    mSprites.push_back(std::make_unique<sf::Sprite>(*texture, textureRect));
    return mSprites.size() - 1;
}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    if (!IsValid(id)) {
        throw std::out_of_range("SpriteManager: invalid SpriteID");
    }
    return *mSprites[id];
}

const sf::Sprite& SpriteManager::GetSprite(SpriteID id) const {
    if (!IsValid(id)) {
        throw std::out_of_range("SpriteManager: invalid SpriteID");
    }
    return *mSprites[id];
}

}  // namespace ECSEngine
