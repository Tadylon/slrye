#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <engine/audio/audio_engine.h>
#include <engine/audio/sound_impl.h>
#include <engine/scene/scene.h>
#include <engine/scene/components.h>

namespace slrye::engine
{

struct AudioEngine::Impl
{
    ma_engine engine{};
    bool is_initialized = false;
};

AudioEngine::AudioEngine()
    : impl_(std::make_unique<Impl>())
{
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

AudioEngine& AudioEngine::instance()
{
    static AudioEngine audio_engine;
    return audio_engine;
}

bool AudioEngine::initialize(std::string* error)
{
    if (initialized_)
    {
        return true;
    }

    ma_engine_config config = ma_engine_config_init();
    ma_result result = ma_engine_init(&config, &impl_->engine);
    if (result != MA_SUCCESS)
    {
        if (error != nullptr)
        {
            *error = "Failed to initialize miniaudio engine with result code " + std::to_string(result);
        }
        return false;
    }

    impl_->is_initialized = true;
    initialized_ = true;
    return true;
}

void AudioEngine::shutdown()
{
    if (initialized_ && impl_->is_initialized)
    {
        ma_engine_uninit(&impl_->engine);
        impl_->is_initialized = false;
        initialized_ = false;
    }
}

bool AudioEngine::is_initialized() const noexcept
{
    return initialized_;
}

void AudioEngine::set_master_volume(float volume) noexcept
{
    if (initialized_)
    {
        ma_engine_set_volume(&impl_->engine, volume);
    }
}

float AudioEngine::master_volume() const noexcept
{
    return initialized_ ? ma_engine_get_volume(&impl_->engine) : 0.0f;
}

void AudioEngine::set_listener_position(const glm::vec3& position) noexcept
{
    if (initialized_)
    {
        ma_engine_listener_set_position(&impl_->engine, 0, position.x, position.y, position.z);
    }
}

void AudioEngine::set_listener_orientation(const glm::vec3& forward, const glm::vec3& up) noexcept
{
    if (initialized_)
    {
        ma_engine_listener_set_direction(&impl_->engine, 0, forward.x, forward.y, forward.z);
        ma_engine_listener_set_world_up(&impl_->engine, 0, up.x, up.y, up.z);
    }
}

std::shared_ptr<Sound> AudioEngine::load_sound(
    const std::filesystem::path& path,
    bool is_3d,
    std::string* error)
{
    if (!initialized_)
    {
        if (error != nullptr)
        {
            *error = "AudioEngine is not initialized";
        }
        return nullptr;
    }

    auto sound = std::make_shared<Sound>();
    sound->impl_ = std::make_unique<Sound::Impl>();

    ma_uint32 flags = MA_SOUND_FLAG_DECODE;
    if (!is_3d)
    {
        flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
    }

    const std::string path_str = path.string();
    ma_result result = ma_sound_init_from_file(
        &impl_->engine,
        path_str.c_str(),
        flags,
        nullptr,
        nullptr,
        &sound->impl_->sound);

    if (result != MA_SUCCESS)
    {
        if (error != nullptr)
        {
            *error = "Failed to load audio file '" + path.string() + "' with result code " + std::to_string(result);
        }
        return nullptr;
    }

    sound->impl_->path = path;
    sound->impl_->is_initialized = true;
    sound->impl_->min_dist = 1.0f;
    sound->impl_->max_dist = 50.0f;

    sound->set_spatialized(is_3d);
    return sound;
}

bool AudioEngine::play_one_shot(const std::filesystem::path& path, float volume)
{
    if (!initialized_)
    {
        return false;
    }

    const std::string path_str = path.string();
    ma_result result = ma_engine_play_sound(&impl_->engine, path_str.c_str(), nullptr);
    return result == MA_SUCCESS;
}

void AudioEngine::update(Scene& scene)
{
    if (!initialized_)
    {
        return;
    }

    // 1. 同步主听音者位置 (AudioListener)
    scene.for_each<AudioListenerComponent, TransformComponent>(
        [this](EntityId /*id*/, AudioListenerComponent& listener, TransformComponent& transform)
        {
            if (listener.active)
            {
                set_listener_position(transform.position);
                const glm::vec3 forward = transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
                const glm::vec3 up = transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
                set_listener_orientation(forward, up);
            }
        });

    // 2. 同步与驱动场景各声源 (AudioSource)
    scene.for_each<AudioSourceComponent, TransformComponent>(
        [this](EntityId /*id*/, AudioSourceComponent& source, TransformComponent& transform)
        {
            if (source.sound_instance == nullptr || source.sound_instance->path() != source.file_path)
            {
                if (!source.file_path.empty())
                {
                    source.sound_instance = load_sound(source.file_path, source.is_3d);
                    if (source.sound_instance != nullptr)
                    {
                        source.sound_instance->set_volume(source.volume);
                        source.sound_instance->set_pitch(source.pitch);
                        source.sound_instance->set_looping(source.looping);
                        source.sound_instance->set_attenuation_distances(source.min_distance, source.max_distance);
                        source.sound_instance->set_position(transform.position);
                        if (source.play_on_awake)
                        {
                            source.sound_instance->play();
                        }
                    }
                }
            }
            else if (source.sound_instance != nullptr && source.sound_instance->is_valid())
            {
                source.sound_instance->set_position(transform.position);
                source.sound_instance->set_volume(source.volume);
                source.sound_instance->set_pitch(source.pitch);
                source.sound_instance->set_looping(source.looping);
                source.sound_instance->set_attenuation_distances(source.min_distance, source.max_distance);
            }
        });
}

}
