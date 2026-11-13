#include "SoundManager.h"

#include <stdexcept>
#include <utility>

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Loads a sound file and registers it under a name.
 *
 * The buffer is created first and moved into the manager's map only after the
 * sf::Sound has been constructed from it. This ordering is important: an
 * sf::Sound stores a pointer to its buffer, so the buffer object must have a
 * stable address before the sound is created.
 *
 * When a name already exists, the sound is replaced before the buffer. The old
 * sound is therefore destroyed while the old buffer is still alive, and the new
 * sound already points at the new buffer when the buffer map is updated.
 *
 * Implemented with the help of AI.
 *
 * @param soundPath Path to the sound file to load.
 * @param soundName Name used later to retrieve or play the sound.
 * @return None.
 * @throws std::invalid_argument when soundName is empty.
 * @throws std::runtime_error when the file cannot be loaded.
 */
//-----------------------------------------------------------------------
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
    // old buffer is still alive. Then replace the buffer, which is safe
    // because the new sound already points at the new buffer.
    mSounds.insert_or_assign(soundName, std::move(sound));
    mSoundBuffers.insert_or_assign(soundName, std::move(buffer));
}

//-----------------------------------------------------------------------
/**
 * @brief Returns the sound registered under a name.
 * @param soundName Name supplied to RegisterSound.
 * @return Reference to the manager-owned sf::Sound.
 * @throws std::out_of_range when soundName has not been registered.
 */
//-----------------------------------------------------------------------
sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    const auto found = mSounds.find(soundName);
    if (found == mSounds.end()) {
        throw std::out_of_range("SoundManager: unknown sound '" + soundName + "'");
    }
    return *found->second;
}

//-----------------------------------------------------------------------
/**
 * @brief Tests whether a name has been registered.
 * @param soundName Name to test.
 * @return true when the name exists in the sound map.
 */
//-----------------------------------------------------------------------
bool SoundManager::HasSound(const std::string& soundName) const {
    return mSounds.find(soundName) != mSounds.end();
}

//-----------------------------------------------------------------------
/**
 * @brief Restarts a registered sound from the beginning.
 * @param soundName Name of the sound to play.
 * @return None.
 * @throws std::out_of_range when soundName has not been registered.
 */
//-----------------------------------------------------------------------
void SoundManager::PlaySound(const std::string& soundName) { GetSound(soundName).play(); }

//-----------------------------------------------------------------------
/**
 * @brief Changes the volume of a registered sound.
 * @param soundName Name of the sound to adjust.
 * @param volume Volume in SFML's 0..100 range.
 * @return None.
 * @throws std::out_of_range when soundName has not been registered.
 */
//-----------------------------------------------------------------------
void SoundManager::SetVolume(const std::string& soundName, float volume) {
    GetSound(soundName).setVolume(volume);
}

}  // namespace ECSEngine
