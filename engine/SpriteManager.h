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

// Owns textures and the sprites that reference them.
//
// Textures are keyed by file path and stored indirectly so that their addresses
// stay stable even when the map grows.  Sprites are stored by index; the index
// is the SpriteID used by SpriteComponent.  The sprite reference returned by
// GetSprite() is stable for the lifetime of the manager (sprites are also held
// indirectly), so systems may cache it if they need to.
class SpriteManager {
public:
    SpriteManager() = default;
    ~SpriteManager() = default;

    SpriteManager(const SpriteManager&) = delete;
    SpriteManager(SpriteManager&&) = delete;
    SpriteManager& operator=(const SpriteManager&) = delete;
    SpriteManager& operator=(SpriteManager&&) = delete;

    // Loads texturePath (once per unique path) and creates a sprite that shows
    // sourceRect from that texture.  The sourceRect is in texture pixels.
    //
    // @throws std::runtime_error when the texture cannot be loaded.
    // @throws std::invalid_argument when texturePath is empty.
    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);

    // Returns the sprite associated with id.
    //
    // @throws std::out_of_range when id was not returned by RegisterTexture.
    sf::Sprite& GetSprite(SpriteID id);

    const sf::Sprite& GetSprite(SpriteID id) const;

    // True when id refers to a registered sprite.
    [[nodiscard]] bool IsValid(SpriteID id) const { return id < mSprites.size(); }

    // Number of registered sprites.
    [[nodiscard]] std::size_t GetSpriteCount() const { return mSprites.size(); }

private:
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> mTextures;
    std::vector<std::unique_ptr<sf::Sprite>> mSprites;
};

}  // namespace ECSEngine
