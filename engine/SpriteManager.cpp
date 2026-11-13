#include "SpriteManager.h"

#include <stdexcept>
#include <utility>

namespace ECSEngine {

namespace {

//-----------------------------------------------------------------------
/**
 * @brief Converts the engine's float Rect into the integer rectangle SFML expects.
 *
 * Texture coordinates are pixel indices, so truncating the floating-point
 * values is intentional here. The engine uses floats for world positions, but
 * a source rectangle must land on whole texels.
 *
 * @param rect Rectangle in engine coordinates.
 * @return Equivalent integer rectangle for sf::Sprite.
 */
//-----------------------------------------------------------------------
sf::IntRect ToIntRect(const Rect& rect) {
    return sf::IntRect({static_cast<int>(rect.topLeft.x), static_cast<int>(rect.topLeft.y)},
                       {static_cast<int>(rect.width), static_cast<int>(rect.height)});
}

}  // namespace

//-----------------------------------------------------------------------
/**
 * @brief Loads a texture if necessary and creates a sprite from a source rect.
 *
 * The texture map stores unique_ptr values so that a texture's address remains
 * stable even if the unordered_map rehashes; sf::Sprite keeps a pointer to the
 * texture and would otherwise be left dangling. The same texture may back many
 * sprites, each with its own source rectangle and SpriteID.
 *
 * Implemented with the help of AI.
 *
 * @param texturePath Path to the image file.
 * @param sourceRect Region of the texture to display; empty means the whole image.
 * @return SpriteID used by SpriteComponent and GetSprite().
 * @throws std::invalid_argument when texturePath is empty.
 * @throws std::runtime_error when the texture cannot be loaded.
 */
//-----------------------------------------------------------------------
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

//-----------------------------------------------------------------------
/**
 * @brief Returns the sprite associated with an id.
 * @param id SpriteID returned by RegisterTexture.
 * @return Mutable reference to the manager-owned sprite.
 * @throws std::out_of_range when id is not a valid sprite id.
 */
//-----------------------------------------------------------------------
sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    if (!IsValid(id)) {
        throw std::out_of_range("SpriteManager: invalid SpriteID");
    }
    return *mSprites[id];
}

//-----------------------------------------------------------------------
/**
 * @brief Const overload of GetSprite().
 * @param id SpriteID returned by RegisterTexture.
 * @return Const reference to the manager-owned sprite.
 * @throws std::out_of_range when id is not a valid sprite id.
 */
//-----------------------------------------------------------------------
const sf::Sprite& SpriteManager::GetSprite(SpriteID id) const {
    if (!IsValid(id)) {
        throw std::out_of_range("SpriteManager: invalid SpriteID");
    }
    return *mSprites[id];
}

}  // namespace ECSEngine
