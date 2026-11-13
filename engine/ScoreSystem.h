#pragma once

#include <cstddef>
#include <string>

#include "ECSEngine.h"

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Keeps the sprite entities that display the score in sync with the
 *        ScoreComponent value.
 *
 * ScoreComponent owns the numeric score, one sprite id for each digit 0-9, and
 * the entity ids used to display each digit. This system does not create those
 * entities; game setup creates them once and places them in the top-left
 * corner. Each frame the system writes the correct digit sprite id into each
 * display entity's SpriteComponent, and SpriteSystem draws the result.
 *
 * The score is clamped to the number of display digits (for example, three
 * digits show 000-999). Negative values are displayed as zero. Missing or
 * removed digit entities are skipped rather than treated as an error, which
 * keeps a partially built scene stable.
 *
 * Implemented with the help of AI.
 */
//-----------------------------------------------------------------------
template <typename... Components>
class ScoreSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("ScoreSystem");
        return name;
    }

    /// @brief Updates every ScoreComponent's digit sprites, then continues.
    tSystemReturn Run(EngineView<Components...>* engine) override {
        UpdateScore(engine->GetEntityManager());
        return kContinue;
    }

    //-------------------------------------------------------------------
    /**
     * @brief Updates the digit sprites for every entity with a ScoreComponent.
     *
     * Public so the core logic can be unit tested without constructing a
     * WindowManager or opening a window.
     *
     * Implemented with the help of AI.
     *
     * @param entities Entity manager holding the score and digit entities.
     * @return None.
     */
    //-------------------------------------------------------------------
    static void UpdateScore(EntityManager<Components...>& entities) {
        for (auto it = entities.begin(); it != entities.end(); ++it) {
            if (!it->IsValid()) {
                continue;
            }

            const EntityID id = it->GetID();
            if (!entities.template HasComponent<ScoreComponent>(id)) {
                continue;
            }

            UpdateDigits(entities, entities.template GetComponent<ScoreComponent>(id));
        }
    }

private:
    //-------------------------------------------------------------------
    /**
     * @brief Writes the current score into the digit display entities.
     *
     * The number of display digits is inferred from the size of
     * ScoreComponent::digitID. Digits are filled from right to left so the
     * ones place is the last entity in the array. A digit entity that has been
     * removed or lacks a SpriteComponent is simply skipped.
     *
     * Implemented with the help of AI.
     *
     * @param entities Entity manager used to access the digit entities.
     * @param score Score component to read and display.
     * @return None.
     */
    //-------------------------------------------------------------------
    static void UpdateDigits(EntityManager<Components...>& entities, ScoreComponent& score) {
        constexpr std::size_t kDigitCount = sizeof(score.digitID) / sizeof(score.digitID[0]);

        int value = score.score < 0 ? 0 : score.score;
        int maxValue = 1;
        for (std::size_t i = 0; i < kDigitCount; ++i) {
            maxValue *= 10;
        }
        if (value >= maxValue) {
            value = maxValue - 1;
        }

        // Fill from the rightmost display entity so that the ones digit is the
        // last element of ScoreComponent::digitID.
        for (std::size_t i = kDigitCount; i-- > 0;) {
            const int digit = value % 10;
            value /= 10;

            const EntityID digitEntity = score.digitID[i];
            if (!entities.IsValid(digitEntity) ||
                !entities.template HasComponent<SpriteComponent>(digitEntity)) {
                continue;
            }

            entities.template GetComponent<SpriteComponent>(digitEntity).textureID =
                score.digitTextureID[digit];
        }
    }
};

}  // namespace ECSEngine
