#include "SoundManager.h"

namespace ECSEngine {

void SoundManager::RegisterSound(const std::string& soundPath, const std::string& soundName) {}

sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    static sf::SoundBuffer dummyBuffer;
    static sf::Sound dummy(dummyBuffer);
    return dummy;
}

}  // namespace ECSEngine
