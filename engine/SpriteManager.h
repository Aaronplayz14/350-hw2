#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

using SpriteID = size_t;

//-----------------------------------------------------------------------
/**
 * @brief Owns textures and the sprites that reference them.
 *
 * Textures are keyed by file path and stored indirectly, so their addresses
 * stay stable even when the texture map grows. That matters because an
 * sf::Sprite keeps a pointer to its texture. Sprites are stored by index and
 * the index is the SpriteID used by SpriteComponent; keeping sprites
 * indirectly also makes the reference returned by GetSprite() stable for the
 * lifetime of the manager.
 *
 * Registering the same texture path twice reuses the already-loaded texture
 * but creates a second sprite with a different SpriteID, which allows many
 * entities to share an image while having independent transforms.
 */
//-----------------------------------------------------------------------
class SpriteManager {
public:
    SpriteManager() = default;
    ~SpriteManager() = default;

    SpriteManager(const SpriteManager&) = delete;
    SpriteManager(SpriteManager&&) = delete;
    SpriteManager& operator=(const SpriteManager&) = delete;
    SpriteManager& operator=(SpriteManager&&) = delete;

    //-------------------------------------------------------------------
    /**
     * @brief Loads a texture if needed and creates a sprite from a source rect.
     *
     * The source rectangle is measured in texture pixels. If either dimension
     * is zero or negative, the entire texture is used, which is convenient for
     * full-image sprites. The texture itself is loaded only once per unique
     * path.
     *
     * @param texturePath Path to the image file.
     * @param sourceRect Region of the texture to display.
     * @return SpriteID used by SpriteComponent and GetSprite().
     * @throws std::invalid_argument when texturePath is empty.
     * @throws std::runtime_error when the texture cannot be loaded.
     */
    //-------------------------------------------------------------------
    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);

    //-------------------------------------------------------------------
    /**
     * @brief Returns the sprite associated with an id.
     * @param id SpriteID returned by RegisterTexture.
     * @return Mutable reference to the manager-owned sprite.
     * @throws std::out_of_range when id was not returned by RegisterTexture.
     */
    //-------------------------------------------------------------------
    sf::Sprite& GetSprite(SpriteID id);

    /// @brief Const overload of GetSprite().
    const sf::Sprite& GetSprite(SpriteID id) const;

    /// @brief True when id refers to a registered sprite.
    [[nodiscard]] bool IsValid(SpriteID id) const { return id < mSprites.size(); }

    /// @brief Number of registered sprites.
    [[nodiscard]] std::size_t GetSpriteCount() const { return mSprites.size(); }

private:
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> mTextures;
    std::vector<std::unique_ptr<sf::Sprite>> mSprites;
};

}  // namespace ECSEngine
