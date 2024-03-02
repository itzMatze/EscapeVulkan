#include "SoundPlayer.hpp"
#include <SDL_mixer.h>

SoundPlayer::SoundPlayer()
{
    for (auto& s : sounds) s = nullptr;
}

SoundPlayer::~SoundPlayer()
{
    for (auto& s : sounds)
    {
        if (s != nullptr) Mix_FreeChunk(s);
        s = nullptr;
    }
    Mix_CloseAudio();
}

void SoundPlayer::init(int32_t channel_count)
{
    // initialize mixer
    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, channel_count, 4096);
    for (int32_t i = 0; i < channel_count; ++i) Mix_Volume(i, MIX_MAX_VOLUME);
    for (uint32_t i = 0; i < SOUND_COUNT; ++i) sounds[i] = Mix_LoadWAV(sound_files[i]);
}

void SoundPlayer::play(SoundPlayer::Sound sound, int32_t channel, int32_t loops)
{
    Mix_PlayChannel(channel, sounds[sound], loops);
}

void SoundPlayer::stop(int32_t channel)
{
    Mix_HaltChannel(channel);
}

void SoundPlayer::set_volume(int32_t channel, int32_t volume)
{
    Mix_Volume(channel, volume);
}

