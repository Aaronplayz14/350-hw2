#pragma once

#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "ECSEngine.h"

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Spawns collectable stars from spawner entities.
 *
 * A SpawnComponent is attached to a map tile marked 'S'. The component stores
 * the star texture, the delay before the next spawn, the delay between spawns,
 * and how many stars are still allowed. SpawnSystem counts those timers down
 * and creates a star with a random direction and speed when a timer reaches
 * zero.
 *
 * Stars are ordinary dynamic entities: LocationComponent, MovementComponent,
 * CollisionComponent, SpriteComponent, and GravityComponent. Gravity pulls
 * them down while CollisionSystem bounces them off the map and off each other,
 * losing some speed with every collision.
 *
 * CreateStar() is public because the level also places one test star by hand
 * (outside the normal spawner path) to exercise wall jumping.
 *
 * Implemented with the help of AI.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class SpawnSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpawnSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        EntityManager<Components...>& entities = engine->GetEntityManager();

        // Collect spawners first: creating a star can grow the entity vector,
        // which would invalidate any iterator we were using to walk it.
        std::vector<EntityID> spawners;
        for (auto it = entities.begin(); it != entities.end(); ++it) {
            if (!it->IsValid()) {
                continue;
            }

            const EntityID id = it->GetID();
            if (entities.template HasComponent<SpawnComponent>(id)) {
                spawners.push_back(id);
            }
        }

        for (const EntityID id : spawners) {
            if (!entities.IsValid(id) ||
                !entities.template HasComponent<SpawnComponent>(id) ||
                !entities.template HasComponent<LocationComponent>(id)) {
                continue;
            }

            SpawnComponent& spawn = entities.template GetComponent<SpawnComponent>(id);
            if (spawn.totalSpawns <= 0) {
                continue;
            }

            if (spawn.timeToNextSpawn > 0) {
                --spawn.timeToNextSpawn;
                continue;
            }

            if (spawn.spawnType != "star") {
                // This system currently understands stars only. Stop spending
                // time on an unknown spawner type rather than ticking forever.
                spawn.totalSpawns = 0;
                continue;
            }

            const LocationComponent& spawnerLocation =
                entities.template GetComponent<LocationComponent>(id);

            // Place the star in the tile above the spawner. Its collision and
            // sprite boxes are inset by 8 pixels, so this also keeps it from
            // starting inside the spawner tile.
            const Point2D starLocation(spawnerLocation.loc.x, spawnerLocation.loc.y - 64.0f);

            static std::mt19937 randomEngine(std::random_device{}());
            static std::uniform_real_distribution<float> angleDistribution(
                0.0f, 2.0f * 3.14159265358979323846f);
            static std::uniform_real_distribution<float> speedDistribution(6.0f, 10.0f);

            const float angle = angleDistribution(randomEngine);
            const float speed = speedDistribution(randomEngine);
            const Point2D velocity(std::cos(angle) * speed, std::sin(angle) * speed);

            CreateStar(entities, spawn.spawnTextureID, starLocation, velocity);

            // Re-fetch the spawner after creating the star; component storage
            // growth can move references inside a storage.
            if (entities.IsValid(id) && entities.template HasComponent<SpawnComponent>(id)) {
                SpawnComponent& updatedSpawn = entities.template GetComponent<SpawnComponent>(id);
                --updatedSpawn.totalSpawns;
                updatedSpawn.timeToNextSpawn = updatedSpawn.timeBetweenSpawns;
            }
        }

        return kContinue;
    }

    //-------------------------------------------------------------------
    /**
     * @brief Creates one collectable star entity.
     *
     * Used by the spawner and by the hand-placed test star in main(). The
     * caller supplies the entity location (the top-left of a 64x64 tile-style
     * cell) and the initial velocity. The visual and collision boxes are inset
     * by 8 pixels so the star does not fill the whole cell.
     *
     * @param entities Entity manager that will own the new star.
     * @param starTexture Registered sprite for the star.
     * @param location Entity location in world coordinates.
     * @param velocity Initial movement velocity.
     * @return Entity id of the new star.
     */
    //-------------------------------------------------------------------
    static EntityID CreateStar(EntityManager<Components...>& entities, SpriteID starTexture,
                               const Point2D& location, const Point2D& velocity) {
        const EntityID star = entities.CreateEntity("star");

        entities.template AddComponent<LocationComponent>(star, LocationComponent{location});
        entities.template AddComponent<MovementComponent>(star, MovementComponent{velocity});

        // Stars fall like the player, then bounce off the level geometry.
        entities.template AddComponent<GravityComponent>(
            star, GravityComponent(Point2D(0.0f, 2.0f)));

        CollisionComponent collision{};
        collision.staticEntity = 0;
        collision.doesDamage = 0;
        collision.collisionBox = Rect(8.0f, 8.0f, 48.0f, 48.0f);
        collision.previousCollisionBox = collision.collisionBox;
        entities.template AddComponent<CollisionComponent>(star, collision);

        SpriteComponent sprite;
        sprite.textureID = starTexture;
        sprite.bounds = Rect(8.0f, 8.0f, 48.0f, 48.0f);
        sprite.worldSpace = true;
        entities.template AddComponent<SpriteComponent>(star, sprite);

        return star;
    }
};

}  // namespace ECSEngine
