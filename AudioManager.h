#pragma once

#include <string>
#include <unordered_map>

class AudioManager
{
public:
    AudioManager();
    ~AudioManager();

    bool Init();
    void Shutdown();

    bool LoadSound(const std::string& name, const std::string& path, bool loop);
    void Play(const std::string& name);
    void Stop(const std::string& name);

    bool IsPlaying(const std::string& name);

    void SetVolume(const std::string& name, float volume);
    void SetGlobalVolume(float volume);

private:
    struct ma_engine* engine;
    bool initialized;

    std::unordered_map<std::string, struct ma_sound*> sounds;
};