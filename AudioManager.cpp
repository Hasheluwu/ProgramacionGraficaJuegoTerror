#define MINIAUDIO_IMPLEMENTATION
#include "vendor/miniaudio.h"

#include "AudioManager.h"

#include <iostream>

AudioManager::AudioManager()
{
    engine = nullptr;
    initialized = false;
}

AudioManager::~AudioManager()
{
    Shutdown();
}

bool AudioManager::Init()
{
    engine = new ma_engine();

    ma_result result = ma_engine_init(NULL, engine);

    if (result != MA_SUCCESS)
    {
        std::cout << "ERROR: No se pudo inicializar el motor de audio." << std::endl;
        delete engine;
        engine = nullptr;
        return false;
    }

    initialized = true;
    return true;
}

void AudioManager::Shutdown()
{
    if (!initialized)
        return;

    for (auto& sound : sounds)
    {
        if (sound.second != nullptr)
        {
            ma_sound_uninit(sound.second);
            delete sound.second;
        }
    }

    sounds.clear();

    if (engine != nullptr)
    {
        ma_engine_uninit(engine);
        delete engine;
        engine = nullptr;
    }

    initialized = false;
}

bool AudioManager::LoadSound(const std::string& name, const std::string& path, bool loop)
{
    if (!initialized || engine == nullptr)
        return false;

    ma_sound* sound = new ma_sound();

    ma_result result = ma_sound_init_from_file(
        engine,
        path.c_str(),
        0,
        NULL,
        NULL,
        sound
    );

    if (result != MA_SUCCESS)
    {
        std::cout << "ERROR: No se pudo cargar el audio: " << path << std::endl;
        delete sound;
        return false;
    }

    ma_sound_set_looping(sound, loop ? MA_TRUE : MA_FALSE);

    sounds[name] = sound;

    return true;
}

void AudioManager::Play(const std::string& name)
{
    auto it = sounds.find(name);

    if (it == sounds.end())
        return;

    ma_sound_seek_to_pcm_frame(it->second, 0);
    ma_sound_start(it->second);
}

void AudioManager::Stop(const std::string& name)
{
    auto it = sounds.find(name);

    if (it == sounds.end())
        return;

    ma_sound_stop(it->second);
    ma_sound_seek_to_pcm_frame(it->second, 0);
}

bool AudioManager::IsPlaying(const std::string& name)
{
    auto it = sounds.find(name);

    if (it == sounds.end())
        return false;

    return ma_sound_is_playing(it->second) == MA_TRUE;
}

void AudioManager::SetVolume(const std::string& name, float volume)
{
    auto it = sounds.find(name);

    if (it == sounds.end())
        return;

    ma_sound_set_volume(it->second, volume);
}

void AudioManager::SetGlobalVolume(float volume)
{
    if (!initialized || engine == nullptr)
        return;

    ma_engine_set_volume(engine, volume);
}