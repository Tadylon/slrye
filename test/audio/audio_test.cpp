#include <engine/audio/audio_engine.h>
#include <engine/scene/scene.h>
#include <engine/scene/components.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("AudioEngine initializes and manages master volume and listener")
{
    auto& audio = slrye::AudioEngine::instance();
    bool ok = audio.initialize();
    REQUIRE(ok);
    REQUIRE(audio.is_initialized());

    audio.set_master_volume(0.75f);
    CHECK(audio.master_volume() == 0.75f);

    audio.set_listener_position({1.0f, 2.0f, 3.0f});
    audio.set_listener_orientation({0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f});

    std::string err;
    auto invalid_sound = audio.load_sound("non_existent_audio_file.wav", true, &err);
    CHECK(invalid_sound == nullptr);
    CHECK_FALSE(err.empty());
}

TEST_CASE("Scene ECS integrates with AudioListener and AudioSource components")
{
    slrye::Scene scene("AudioScene");

    auto listener_entity = scene.create_entity("MainListener");
    auto& listener = listener_entity.add<slrye::AudioListenerComponent>();
    listener.active = true;

    auto& transform = listener_entity.get<slrye::TransformComponent>();
    transform.position = {5.0f, 0.0f, 0.0f};

    auto source_entity = scene.create_entity("BGM_Source");
    auto& source = source_entity.add<slrye::AudioSourceComponent>();
    source.file_path = "non_existent.ogg";
    source.volume = 0.5f;
    source.looping = true;
    source.is_3d = true;

    REQUIRE(listener_entity.has<slrye::AudioListenerComponent>());
    REQUIRE(source_entity.has<slrye::AudioSourceComponent>());

    // AudioEngine::update processes scene components cleanly
    slrye::AudioEngine::instance().update(scene);
}
