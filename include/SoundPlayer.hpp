#include <array>
#include <SDL2/SDL_mixer.h>

class SoundPlayer
{
public:
    enum Sound
    {
        SPACESHIP_THRUST = 0,
        SPACESHIP_CRASH = 1,
        GAME_OVER = 2,
        SOUND_COUNT
    };

    SoundPlayer();
    ~SoundPlayer();
    void init(int32_t channel_count = 1);
    void play(SoundPlayer::Sound sound, int32_t channel = 0, int32_t loops = 1);
    void stop(int32_t channel = 0);
    void set_volume(int32_t channel = 0, int32_t volume = 0);
private:
    std::array<Mix_Chunk*, SOUND_COUNT> sounds;
    static constexpr std::array<const char*, SOUND_COUNT> sound_files = {
        "../assets/sounds/spaceship_thrust_1.wav",
        "../assets/sounds/spaceship_crash.wav",
        "../assets/sounds/game_over.wav"
    };
};
