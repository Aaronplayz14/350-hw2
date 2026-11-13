#pragma once

#include <algorithm>
#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

/// @brief Contact flags from the latest collision pass, static/damage tags,
/// and collision bounds. collisionBox is entity-relative; previousCollisionBox
/// is saved in world coordinates before movement. resting is a star status,
/// not a request to skip gravity or freeze the entity permanently.
struct CollisionComponent {
    uint8_t staticEntity : 1;
    uint8_t doesDamage : 1;
    uint8_t onGround : 1;
    uint8_t onCeiling : 1;
    uint8_t onLeftWall : 1;
    uint8_t onRightWall : 1;
    uint8_t resting : 1;        // Fully settled after floor collision resolution
    Rect collisionBox;          // The bounding box for collision detection
    Rect previousCollisionBox;  // The previous bounding box for collision detection
};

//-----------------------------------------------------------------------
/**
 * @brief Builds static map collision data using the dictionary's local bounds.
 *
 * Lowercase x is the spike/saw hazard in world.map. Its supplied rectangle
 * overhangs the sprite by one pixel on both sides; clip it to the tile so a
 * player sliding beside the neighboring wall cannot touch invisible edges.
 * Spike/saw x and uppercase X tiles deal damage and respawn the player on
 * contact. Other solids use their dictionary bounds unchanged.
 *
 * @param symbol Map dictionary character.
 * @param bounds Entity-relative dictionary hitbox.
 * @param tileWidth Width of the tile's drawn sprite in world units.
 * @param tileHeight Height of the tile's drawn sprite in world units.
 * @return Static collision data; the snapshot pass initializes previous world bounds.
 */
//-----------------------------------------------------------------------
inline CollisionComponent MakeMapCollision(char symbol, const Rect& bounds,
                                           float tileWidth, float tileHeight) {
    CollisionComponent collision{};
    collision.staticEntity = 1;
    collision.doesDamage = symbol == 'x' || symbol == 'X';
    collision.collisionBox = bounds;
    if (symbol == 'x') {
        const float left = std::clamp(bounds.topLeft.x, 0.0F, tileWidth);
        const float top = std::clamp(bounds.topLeft.y, 0.0F, tileHeight);
        const float right = std::clamp(bounds.topLeft.x + bounds.width, left, tileWidth);
        const float bottom = std::clamp(bounds.topLeft.y + bounds.height, top, tileHeight);
        collision.collisionBox = Rect(left, top, right - left, bottom - top);
    }
    collision.previousCollisionBox = collision.collisionBox;
    return collision;
}

}  // namespace ECSEngine
