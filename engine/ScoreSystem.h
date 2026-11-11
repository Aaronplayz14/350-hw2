#pragma once

#include <cstddef>
#include <string>

#include "ECSEngine.h"

namespace ECSEngine {

// Keeps the sprite entities that display the score in sync with
// ScoreComponent::score.
//
// ScoreComponent owns the score value, the sprite ids for the digits 0-9, and
// the entities used to display each digit.  This system only changes those
// display entities' SpriteComponent::textureID; the entities themselves (and
// their locations) are created by the game setup code.
template <typename... Components>
class ScoreSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("ScoreSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        UpdateScore(engine->GetEntityManager());
        return kContinue;
    }

    // Public so it can be unit tested without constructing a WindowManager.
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
