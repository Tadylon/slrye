#include <engine/audio/sound.h>
#include <engine/audio/sound_impl.h>

namespace slrye::engine
{

Sound::Sound() = default;

Sound::~Sound()
{
    if (impl_ && impl_->is_initialized)
    {
        ma_sound_uninit(&impl_->sound);
        impl_->is_initialized = false;
    }
}

Sound::Sound(Sound&&) noexcept = default;
Sound& Sound::operator=(Sound&&) noexcept = default;

void Sound::play()
{
    if (impl_ && impl_->is_initialized)
    {
        ma_sound_start(&impl_->sound);
    }
}

void Sound::pause()
{
    if (impl_ && impl_->is_initialized)
    {
        ma_sound_stop(&impl_->sound);
    }
}

void Sound::stop()
{
    if (impl_ && impl_->is_initialized)
    {
        ma_sound_stop(&impl_->sound);
        ma_sound_seek_to_pcm_frame(&impl_->sound, 0);
    }
}

bool Sound::is_playing() const noexcept
{
    return impl_ && impl_->is_initialized && ma_sound_is_playing(&impl_->sound) == MA_TRUE;
}

bool Sound::is_looping() const noexcept
{
    return impl_ && impl_->is_initialized && ma_sound_is_looping(&impl_->sound) == MA_TRUE;
}

void Sound::set_looping(bool looping) noexcept
{
    if (impl_ && impl_->is_initialized)
    {
        ma_sound_set_looping(&impl_->sound, looping ? MA_TRUE : MA_FALSE);
    }
}

float Sound::volume() const noexcept
{
    return (impl_ && impl_->is_initialized) ? ma_sound_get_volume(&impl_->sound) : 0.0f;
}

void Sound::set_volume(float volume) noexcept
{
    if (impl_ && impl_->is_initialized)
    {
        ma_sound_set_volume(&impl_->sound, volume);
    }
}

float Sound::pitch() const noexcept
{
    return (impl_ && impl_->is_initialized) ? ma_sound_get_pitch(&impl_->sound) : 1.0f;
}

void Sound::set_pitch(float pitch) noexcept
{
    if (impl_ && impl_->is_initialized)
    {
        ma_sound_set_pitch(&impl_->sound, pitch);
    }
}

bool Sound::is_spatialized() const noexcept
{
    return impl_ && impl_->is_initialized && ma_sound_is_spatialization_enabled(&impl_->sound) == MA_TRUE;
}

void Sound::set_spatialized(bool spatialized) noexcept
{
    if (impl_ && impl_->is_initialized)
    {
        ma_sound_set_spatialization_enabled(&impl_->sound, spatialized ? MA_TRUE : MA_FALSE);
    }
}

glm::vec3 Sound::position() const noexcept
{
    if (impl_ && impl_->is_initialized)
    {
        ma_vec3f pos = ma_sound_get_position(&impl_->sound);
        return {pos.x, pos.y, pos.z};
    }
    return {0.0f, 0.0f, 0.0f};
}

void Sound::set_position(const glm::vec3& position) noexcept
{
    if (impl_ && impl_->is_initialized)
    {
        ma_sound_set_position(&impl_->sound, position.x, position.y, position.z);
    }
}

void Sound::set_attenuation_distances(float min_distance, float max_distance) noexcept
{
    if (impl_ && impl_->is_initialized)
    {
        impl_->min_dist = min_distance;
        impl_->max_dist = max_distance;
        ma_sound_set_min_distance(&impl_->sound, min_distance);
        ma_sound_set_max_distance(&impl_->sound, max_distance);
    }
}

float Sound::min_distance() const noexcept
{
    return impl_ ? impl_->min_dist : 1.0f;
}

float Sound::max_distance() const noexcept
{
    return impl_ ? impl_->max_dist : 50.0f;
}

const std::filesystem::path& Sound::path() const noexcept
{
    static const std::filesystem::path empty_path;
    return impl_ ? impl_->path : empty_path;
}

bool Sound::is_valid() const noexcept
{
    return impl_ && impl_->is_initialized;
}

}
