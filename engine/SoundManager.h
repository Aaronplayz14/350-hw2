#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include <SFML/Audio.hpp>

namespace ECSEngine {

// Owns sound buffers and the sf::Sound objects that play them.
//
// Sounds are registered once (usually during engine setup) and then requested
// by name.  Buffers are kept alive for the lifetime of the manager because an
// sf::Sound only stores a pointer to its buffer.  The reference returned by
// GetSound() is also owned by the manager and remains valid until the manager
// is destroyed or the same name is registered again.
class SoundManager {
public:
    SoundManager() = default;
    ~SoundManager() = default;

    SoundManager(const SoundManager&) = delete;
    SoundManager(SoundManager&&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    SoundManager& operator=(SoundManager&&) = delete;

    // Loads soundPath and associates it with soundName.  Re-registering a name
    // replaces the previous sound.
    //
    // @throws std::runtime_error when the file cannot be loaded.
    // @throws std::invalid_argument when soundName is empty.
    void RegisterSound(const std::string& soundPath, const std::string& soundName);

    // Returns the sound registered under soundName.
    //
    // @throws std::out_of_range when soundName has not been registered.
    sf::Sound& GetSound(const std::string& soundName);

    // True when soundName is registered.
    [[nodiscard]] bool HasSound(const std::string& soundName) const;

    // Convenience: restarts the named sound from the beginning.
    void PlaySound(const std::string& soundName);

    // Sets the volume (0..100) of the named sound.
    void SetVolume(const std::string& soundName, float volume);

    // Number of registered sounds.
    [[nodiscard]] std::size_t GetSoundCount() const { return mSounds.size(); }

private:
    // Buffers must outlive the sounds that point at them, so they are declared
    // first and only destroyed after mSounds (destruction is reverse order).
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> mSoundBuffers;
    std::unordered_map<std::string, std::unique_ptr<sf::Sound>> mSounds;
};

}  // namespace ECSEngine
