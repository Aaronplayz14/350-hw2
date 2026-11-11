#include "SoundManager.h"

#include <stdexcept>
#include <utility>

namespace ECSEngine {

void SoundManager::RegisterSound(const std::string& soundPath, const std::string& soundName) {
    if (soundName.empty()) {
        throw std::invalid_argument("SoundManager::RegisterSound requires a non-empty name");
    }

    auto buffer = std::make_unique<sf::SoundBuffer>();
    if (!buffer->loadFromFile(soundPath)) {
        throw std::runtime_error("SoundManager could not load sound: " + soundPath);
    }

    // Create the sound before moving the buffer into the map so that the sound
    // points at the final, stable buffer object.
    auto sound = std::make_unique<sf::Sound>(*buffer);

    // Replace the sound first: the old sound (if any) is destroyed while its
    // old buffer is still alive.  Then replace the buffer, which is safe
    // because the new sound already points at the new buffer.
    mSounds.insert_or_assign(soundName, std::move(sound));
    mSoundBuffers.insert_or_assign(soundName, std::move(buffer));
}

sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    const auto found = mSounds.find(soundName);
    if (found == mSounds.end()) {
        throw std::out_of_range("SoundManager: unknown sound '" + soundName + "'");
    }
    return *found->second;
}

bool SoundManager::HasSound(const std::string& soundName) const {
    return mSounds.find(soundName) != mSounds.end();
}

void SoundManager::PlaySound(const std::string& soundName) { GetSound(soundName).play(); }

void SoundManager::SetVolume(const std::string& soundName, float volume) {
    GetSound(soundName).setVolume(volume);
}

}  // namespace ECSEngine
