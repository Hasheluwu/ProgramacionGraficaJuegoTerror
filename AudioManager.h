#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

class AudioManager
{
public:
    AudioManager();
    ~AudioManager();

    bool Init();
    void Shutdown();

    // 2D (sin cambios)
    bool LoadSound(const std::string& name, const std::string& path, bool loop);
    void Play(const std::string& name);
    void Stop(const std::string& name);
    bool IsPlaying(const std::string& name);
    void SetVolume(const std::string& name, float volume);
    void SetGlobalVolume(float volume);

    // 3D (nuevo)
    bool LoadSound3D(const std::string& name, const std::string& path, bool loop);
    void Play3D(const std::string& name, const glm::vec3& position, float volume = 1.0f, float minDistance = 1.0f, float maxDistance = 30.0f);
    void SetSoundPosition(const std::string& name, const glm::vec3& position);
    void SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

private:
    struct ma_engine* engine;
    bool initialized;

    std::unordered_map<std::string, struct ma_sound*> sounds;
};