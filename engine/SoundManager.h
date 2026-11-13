#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include <SFML/Audio.hpp>

namespace ECSEngine {

//-----------------------------------------------------------------------
/**
 * @brief Owns sound buffers and the sf::Sound objects that play them.
 *
 * Sounds are registered once, usually during engine setup, and then requested
 * by name from gameplay code. The manager keeps every buffer alive for its
 * whole lifetime because an sf::Sound only stores a pointer to its buffer; if
 * the buffer were destroyed the sound would refer to freed memory. Sounds and
 * buffers are stored indirectly so that the addresses used by sf::Sound remain
 * stable even when the maps rehash.
 *
 * GetSound() returns a reference owned by the manager. The reference remains
 * valid until the manager is destroyed or the same name is registered again.
 */
//-----------------------------------------------------------------------
class SoundManager {
public:
    SoundManager() = default;
    ~SoundManager() = default;

    SoundManager(const SoundManager&) = delete;
    SoundManager(SoundManager&&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    SoundManager& operator=(SoundManager&&) = delete;

    //-------------------------------------------------------------------
    /**
     * @brief Loads a sound file and associates it with a name.
     *
     * Re-registering a name replaces the previous sound and buffer. All names
     * are independent, so several gameplay events may point at the same file
     * or at different files.
     *
     * @param soundPath Path to the .ogg (or other SFML-supported) file.
     * @param soundName Name used later with GetSound/PlaySound.
     * @return None.
     * @throws std::runtime_error when the file cannot be loaded.
     * @throws std::invalid_argument when soundName is empty.
     */
    //-------------------------------------------------------------------
    void RegisterSound(const std::string& soundPath, const std::string& soundName);

    //-------------------------------------------------------------------
    /**
     * @brief Returns the sound registered under a name.
     * @param soundName Name supplied to RegisterSound.
     * @return Reference to the manager-owned sf::Sound.
     * @throws std::out_of_range when soundName has not been registered.
     */
    //-------------------------------------------------------------------
    sf::Sound& GetSound(const std::string& soundName);

    /// @brief True when soundName is registered.
    [[nodiscard]] bool HasSound(const std::string& soundName) const;

    /// @brief Restarts the named sound from the beginning.
    void PlaySound(const std::string& soundName);

    /// @brief Sets the volume (0..100) of the named sound.
    void SetVolume(const std::string& soundName, float volume);

    /// @brief Number of registered sounds.
    [[nodiscard]] std::size_t GetSoundCount() const { return mSounds.size(); }

private:
    // Buffers must outlive the sounds that point at them, so they are declared
    // first and only destroyed after mSounds (destruction is reverse order).
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> mSoundBuffers;
    std::unordered_map<std::string, std::unique_ptr<sf::Sound>> mSounds;
};

}  // namespace ECSEngine
